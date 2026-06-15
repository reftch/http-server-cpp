#ifndef SSL_SERVER_H_
#define SSL_SERVER_H_

#ifdef HTTP_OPENSSL_SUPPORT
#include <openssl/err.h>
#include <openssl/ssl.h>

#include <unordered_map>

#include "server.h"

namespace http {

    class SSLServer : public Server {
       public:
        struct ClientConnection {
            int fd = -1;
            SSL* ssl = nullptr;
            bool handshake_completed = false;
        };

        /**
         * Constructor
         */
        SSLServer(const std::string& host, const int& port, const std::string& cert_file, const std::string& key_file)
            : Server(host, port), cert_file_(cert_file), key_file_(key_file), ssl_ctx_(nullptr) {
            SSL_library_init();
            SSL_load_error_strings();
            OpenSSL_add_ssl_algorithms();
        }

        /**
         * Destructor
         */
        ~SSLServer() override {
            stop();
            if (ssl_ctx_) {
                SSL_CTX_free(ssl_ctx_);
                ssl_ctx_ = nullptr;
            }

            EVP_cleanup();
        }

        /**
         * Start HTTPS server
         */
        int start() override {
            if (!initializeSSL()) {
                return 1;
            }

            sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd_ < 0) {
                log.error("Socket creation failed");
                return 2;
            }

            int opt = 1;
            if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
                log.error("setsockopt failed");
                return 3;
            }

            int flags = fcntl(sockfd_, F_GETFL, 0);
            fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK);
            sockaddr_in server_addr;
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port_);
            inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr);

            if (bind(sockfd_, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                log.error("Bind failed");
                return 4;
            }

            if (listen(sockfd_, SOMAXCONN) < 0) {
                log.error("Listen failed");
                return 5;
            }

            running_ = true;

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);

            log.info("Server started on https://{}:{} in {} ", host_, port_, duration);

            handleRequests();

            return 0;
        };

        /**
         * Stop HTTPS server
         */
        void stop() override {
            running_ = false;

            std::vector<int> clients;
            std::unordered_map<int, ClientConnection>::iterator it;
            for (it = ssl_clients_.begin(); it != ssl_clients_.end(); ++it) {
                clients.push_back(it->first);
            }

            size_t i;
            for (i = 0; i < clients.size(); ++i) {
                closeClient(clients[i]);
            }

            if (sockfd_ != -1) {
                close(sockfd_);
                sockfd_ = -1;
            }

            log.info("HTTPS server stopped");
        }

       protected:
        /**
         * HTTPS request handling loop
         */
        void handleRequests() override {
            std::vector<struct pollfd> pollfds;
            while (running_) {
                // Clear and rebuild pollfds vector
                pollfds.clear();

                // Add server socket
                struct pollfd server_pollfd;
                server_pollfd.fd = sockfd_;
                server_pollfd.events = POLLIN;
                server_pollfd.revents = 0;
                pollfds.push_back(server_pollfd);

                std::unordered_map<int, ClientConnection>::iterator it;

                for (it = ssl_clients_.begin(); it != ssl_clients_.end(); ++it) {
                    pollfd pfd;

                    pfd.fd = it->first;
                    pfd.events = POLLIN;
                    pfd.revents = 0;

                    pollfds.push_back(pfd);
                }

                int activity = poll(&pollfds[0], pollfds.size(), POLL_TIMEOUT);
                if (activity < 0) {
                    // if (errno == EINTR) {
                    //     continue;
                    // }
                    log.warning("poll failed");
                    continue;
                }

                //
                // ACCEPT NEW CLIENT
                //
                if (pollfds[0].revents & POLLIN) {
                    int clientfd = accept(sockfd_, (struct sockaddr*)NULL, NULL);
                    if (clientfd < 0) {
                        log.warning("Accepted error");
                        continue;
                    }
                    // Set non-blocking mode for client socket
                    int flags = fcntl(clientfd, F_GETFL, 0);
                    fcntl(clientfd, F_SETFL, flags | O_NONBLOCK);

                    SSL* ssl = SSL_new(ssl_ctx_);
                    if (!ssl) {
                        close(clientfd);
                        continue;
                    }

                    SSL_set_fd(ssl, clientfd);

                    ClientConnection client;

                    client.fd = clientfd;
                    client.ssl = ssl;
                    client.handshake_completed = false;

                    ssl_clients_[clientfd] = client;
                }

                //
                // HANDLE CLIENTS
                //
                size_t i;

                std::vector<int> dead_clients;
                for (i = 0; i < pollfds.size(); ++i) {
                    if (!(pollfds[i].revents & POLLIN)) {
                        continue;
                    }

                    int fd = pollfds[i].fd;
                    if (ssl_clients_.find(fd) == ssl_clients_.end()) {
                        continue;
                    }

                    ClientConnection& client = ssl_clients_[fd];

                    //
                    // HANDSHAKE
                    //
                    if (!client.handshake_completed) {
                        int ret = SSL_accept(client.ssl);
                        if (ret == 1) {
                            client.handshake_completed = true;
                            // log.Info("TLS handshake completed FD={}", fd);
                        } else {
                            int err = SSL_get_error(client.ssl, ret);
                            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                                continue;
                            }

                            log.warning("TLS handshake failed FD={}", fd);
                            dead_clients.push_back(fd);
                            continue;
                        }
                    }

                    //
                    // READ REQUEST
                    //
                    char buffer[READ_BUFFER_SIZE];
                    ssize_t nread = SSLRead(client, buffer, sizeof(buffer));
                    if (nread > 0) {
                        performRequest(fd, buffer, nread);
                    } else if (nread < 0) {
                        dead_clients.push_back(fd);
                    }
                }

                //
                // CLEANUP
                //
                for (i = 0; i < dead_clients.size(); ++i) {
                    closeClient(dead_clients[i]);
                }
            }
        };

       private:
        //
        // SSL CONFIG
        //
        std::string cert_file_;
        std::string key_file_;

        SSL_CTX* ssl_ctx_;

        //
        // SSL CLIENT STORAGE
        //
        std::unordered_map<int, ClientConnection> ssl_clients_;

        /**
         * Initialize SSL context
         */
        bool initializeSSL() {
            const SSL_METHOD* method = TLS_server_method();

            ssl_ctx_ = SSL_CTX_new(method);
            if (!ssl_ctx_) {
                log.error("Failed to create SSL context");
                ERR_print_errors_fp(stderr);
                return false;
            }

            SSL_CTX_set_min_proto_version(ssl_ctx_, TLS1_2_VERSION);
            if (SSL_CTX_use_certificate_file(ssl_ctx_, cert_file_.c_str(), SSL_FILETYPE_PEM) <= 0) {
                log.error("Failed to load certificate file");
                ERR_print_errors_fp(stderr);
                return false;
            }

            if (SSL_CTX_use_PrivateKey_file(ssl_ctx_, key_file_.c_str(), SSL_FILETYPE_PEM) <= 0) {
                log.error("Failed to load private key file");
                ERR_print_errors_fp(stderr);
                return false;
            }

            if (!SSL_CTX_check_private_key(ssl_ctx_)) {
                log.error("Private key mismatch");
                return false;
            }

            return true;
        }

        /**
         * Close + cleanup client
         */
        void closeClient(int fd) {
            std::unordered_map<int, ClientConnection>::iterator it = ssl_clients_.find(fd);
            if (it == ssl_clients_.end()) {
                return;
            }

            ClientConnection& client = it->second;
            if (client.ssl) {
                // SSL_shutdown(client.ssl);
                SSL_free(client.ssl);
                client.ssl = NULL;
            }

            if (client.fd != -1) {
                close(client.fd);
                client.fd = -1;
            }

            ssl_clients_.erase(it);
        }

        /**
         * Perform HTTPS request
         */
        void performRequest(const int sd, const char* buffer, const ssize_t nread) override {
            if (ssl_clients_.find(sd) == ssl_clients_.end()) {
                return;
            }

            ClientConnection& client = ssl_clients_[sd];
            std::string raw_request(buffer, nread);
            Request req(raw_request);
            std::string response = handleRoute(req);

            SSLWrite(client, response.c_str(), response.size());
        }

        /**
         * SSL read wrapper
         */
        ssize_t SSLRead(ClientConnection& client, char* buffer, size_t len) {
            int ret = SSL_read(client.ssl, buffer, len - 1);
            if (ret > 0) {
                buffer[ret] = '\0';
                return ret;
            }

            int err = SSL_get_error(client.ssl, ret);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                return 0;
            }

            return -1;
        }

        /**
         * SSL write wrapper
         */
        ssize_t SSLWrite(ClientConnection& client, const char* buffer, size_t len) {
            int ret = SSL_write(client.ssl, buffer, len);
            if (ret > 0) {
                return ret;
            }

            return -1;
        }
    };

}  // namespace http

#endif  // SSL_SERVER_H_
#endif