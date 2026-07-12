#ifndef SSL_SERVER_H_
#define SSL_SERVER_H_

#ifdef HTTP_OPENSSL_SUPPORT
#include <openssl/err.h>
#include <openssl/ssl.h>

#include <algorithm>
#include <ranges>
// #include <string>
// #include <mutex>
// #include <unordered_map>

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
            isHttps = true;
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

            auto end_ = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_ - start_);

            log.info("Server started on https://{}:{} in {}", host_, port_, duration);
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
         * Handles the server's listening socket: accepts new connections,
         * sets non-blocking mode, and initializes SSL context.
         */
        void handleListenSocket(const pollfd&) override {
            // We use sockfd_ directly as per original logic
            int clientfd = accept(sockfd_, nullptr, nullptr);
            if (clientfd < 0) {
                log.warning("Accepted error");
                return;
            }

            // Set non-blocking mode
            int flags = fcntl(clientfd, F_GETFL, 0);
            fcntl(clientfd, F_SETFL, flags | O_NONBLOCK);

            if (SSL* ssl = SSL_new(ssl_ctx_)) {
                SSL_set_fd(ssl, clientfd);
                ssl_clients_[clientfd] = ClientConnection{clientfd, ssl, false};
            } else {
                close(clientfd);
            }
        }

        /**
         * Handles an individual client: performs TLS handshake if necessary,
         * or reads incoming data. Adds FD to closedSockets if a fatal error occurs.
         */
        void handleClientSocket(const pollfd& pfd, std::vector<int>& closedSockets) override {
            int fd = pfd.fd;
            auto it = ssl_clients_.find(fd);
            if (it == ssl_clients_.end()) return;

            ClientConnection& client = it->second;

            // handle Handshake Phase
            if (!client.handshake_completed) {
                int ret = SSL_accept(client.ssl);
                if (ret == 1) {
                    client.handshake_completed = true;
                    log.debug("TLS handshake completed FD={}", fd);
                } else {
                    int err = SSL_get_error(client.ssl, ret);
                    if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
                        log.debug("TLS handshake failed FD={} and will close", fd);
                        closedSockets.push_back(fd);
                    }
                    return;  // Exit early if we are still waiting for handshake
                }
            }

            // handle Data Phase (only if handshake is complete)
            char buffer[READ_BUFFER_SIZE];
            ssize_t nread = SSLRead(client, buffer, sizeof(buffer));
            if (nread > 0) {
                performRequest(fd, buffer, nread);
            } else if (nread < 0) {
                closedSockets.push_back(fd);
            }
        }

        /**
         * HTTPS request handling loop
         */
        void handleRequests() override {
            while (running_) {
                // Build pollfds using functional composition
                auto client_view = ssl_clients_ | std::views::transform([](const auto& pair) {
                                       return pollfd{pair.first, POLLIN, 0};
                                   });

                std::vector<pollfd> pollfds;
                pollfds.reserve(1 + ssl_clients_.size());
                pollfds.push_back({sockfd_, POLLIN, 0});
                pollfds.insert(pollfds.end(), client_view.begin(), client_view.end());

                // Wait for activity
                int activity = poll(pollfds.data(), static_cast<int>(pollfds.size()), POLL_TIMEOUT);
                if (activity < 0) {
                    if (errno == EINTR) continue;
                    log.warning("poll failed");
                    continue;
                }

                // Identify sockets with activity via a view pipeline
                std::vector<int> dead_clients;
                auto ready_fds = pollfds | std::views::filter([](const pollfd& p) {
                                     return (p.revents & POLLIN);
                                 });

                // Dispatch: Distinguish between Listen Socket and Client Sockets
                std::ranges::for_each(ready_fds, [&](const pollfd& pfd) {
                    if (pfd.fd == sockfd_) {
                        handleListenSocket(pfd);
                    } else {
                        handleClientSocket(pfd, dead_clients);
                    }
                });

                // Cleanup: Close any sockets marked for removal
                std::ranges::for_each(dead_clients, [this](int fd) {
                    closeClient(fd);
                });
            }
        }

        void performRequest(const int sd, const char* buffer, const ssize_t nread) override {
            std::string raw_request(buffer, nread);
            // Parse the request line to find method and path
            http::Request req(raw_request);

            // create response
            ClientConnection& client = ssl_clients_[sd];
            Response res(default_headers_, client.ssl);

            // Pre-routing handler
            if (pre_routing_handler_) {
                pre_routing_handler_(req, res);
            }

            // is websocket requests
            auto opcode = getWebSocketFrame(raw_request);
            if (opcode.has_value()) {
                log.debug("Websocket status: {}", toWsOpcodeString(opcode.value()));
                auto route = getWsRouteBySocketId(sd);
                if (route.has_value()) {
                    if (opcode.value() == WsOpcode::Close) {
                        return;
                    }

                    ClientConnection& client = ssl_clients_[sd];
                    WebSocket ws(sd, client.ssl, raw_request, route->query);
                    route->handler(ws);
                }
                return;
            }

            // HTTP websocket handshake
            if (processWebsocketHandshake(sd, req)) {
                log.info("Norm path: {}", req.normalize_path());
                updateWsRoute(req.normalize_path(), req.query(), sd);
                return;
            }

            // handle request
            std::string body = handleRoute(req, res);
            // send server response
            sendResponse(sd, body);
        }

       private:
        //
        // SSL CONFIG
        //
        std::string cert_file_;
        std::string key_file_;

        std::vector<ClientConnection> ssl_connections_;
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
                log.debug("SSL client is not found");
                return;
            }

            ClientConnection& client = it->second;
            if (client.ssl) {
                SSL_free(client.ssl);
                client.ssl = nullptr;
            }

            if (client.fd != -1) {
                close(client.fd);
                client.fd = -1;
            }

            ssl_clients_.erase(it);
        }

        /**
         * Send response
         */
        bool sendResponse(const int sd, std::string& body) override {
            ClientConnection& client = ssl_clients_[sd];
            std::string_view data = body;  // Use string_view for easier slicing

            // Recursive lambda definition
            std::function<bool()> sendAll = [&]() -> bool {
                ssize_t written = SSLWrite(client, data.data(), data.size());

                if (written == -1) {
                    // Check if it's a temporary error
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        return sendAll();  // Recurse with the same data
                    } else {
                        log.warning("Error writing response body: {}", strerror(errno));
                        return false;  // Hard error
                    }
                }

                // Advance the view by 'written' bytes
                data.remove_prefix(written);

                if (data.empty()) {
                    return true;  // All data sent successfully
                }

                return sendAll();  // Recurse with remaining data
            };

            return sendAll();
        }

        /**
         * SSL read wrapper
         */
        ssize_t SSLRead(ClientConnection& client, char* buffer, size_t len) {
            ssize_t ret = SSL_read(client.ssl, buffer, len - 1);
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
            ssize_t ret = SSL_write(client.ssl, buffer, len);
            if (ret > 0) {
                return ret;
            }

            return -1;
        }
    };

}  // namespace http

#endif
#endif