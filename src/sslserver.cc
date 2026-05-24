#include "sslserver.h"

namespace http {

    bool SSLServer::InitializeSSL() {
        const SSL_METHOD* method = TLS_server_method();

        ssl_ctx_ = SSL_CTX_new(method);
        if (!ssl_ctx_) {
            log.Error("Failed to create SSL context");
            ERR_print_errors_fp(stderr);
            return false;
        }

        SSL_CTX_set_min_proto_version(ssl_ctx_, TLS1_2_VERSION);
        if (SSL_CTX_use_certificate_file(ssl_ctx_, cert_file_.c_str(), SSL_FILETYPE_PEM) <= 0) {
            log.Error("Failed to load certificate file");
            ERR_print_errors_fp(stderr);
            return false;
        }

        if (SSL_CTX_use_PrivateKey_file(ssl_ctx_, key_file_.c_str(), SSL_FILETYPE_PEM) <= 0) {
            log.Error("Failed to load private key file");
            ERR_print_errors_fp(stderr);
            return false;
        }

        if (!SSL_CTX_check_private_key(ssl_ctx_)) {
            log.Error("Private key mismatch");
            return false;
        }

        return true;
    }

    void SSLServer::CloseClient(int fd) {
        std::unordered_map<int, ClientConnection>::iterator it = ssl_clients_.find(fd);
        if (it == ssl_clients_.end()) {
            return;
        }

        ClientConnection& client = it->second;
        if (client.ssl) {
            SSL_shutdown(client.ssl);
            SSL_free(client.ssl);
            client.ssl = NULL;
        }

        if (client.fd != -1) {
            close(client.fd);
            client.fd = -1;
        }

        ssl_clients_.erase(it);
    }

    int SSLServer::Start() {
        if (!InitializeSSL()) {
            return 1;
        }

        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ < 0) {
            log.Error("Socket creation failed");
            return 2;
        }

        int opt = 1;
        if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            log.Error("setsockopt failed");
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
            log.Error("Bind failed");
            return 4;
        }

        if (listen(sockfd_, kMAX_CONNS) < 0) {
            log.Error("Listen failed");
            return 5;
        }

        running_ = true;

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);

        log.Info("Server started on https://{}:{} in {} ", host_, port_, duration);

        HandleRequests();

        return 0;
    }

    void SSLServer::HandleRequests() {
        while (running_) {
            std::vector<pollfd> pollfds;

            pollfd server_pollfd;

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

            int activity = poll(&pollfds[0], pollfds.size(), kCONNECTION_TIMEOUT_SECOND * 1000);
            if (activity < 0) {
                if (errno == EINTR) {
                    continue;
                }
                log.Warning("poll failed");
                continue;
            }

            //
            // ACCEPT NEW CLIENT
            //
            if (pollfds[0].revents & POLLIN) {
                int clientfd = accept(sockfd_, NULL, NULL);

                if (clientfd >= 0) {
                    int client_flags = fcntl(clientfd, F_GETFL, 0);
                    fcntl(clientfd, F_SETFL, client_flags | O_NONBLOCK);

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
                    // log.Info("Client connected FD={}", clientfd);
                }
            }

            //
            // HANDLE CLIENTS
            //
            size_t i;

            std::vector<int> dead_clients;
            for (i = 1; i < pollfds.size(); ++i) {
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

                        log.Warning("TLS handshake failed FD={}", fd);
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
                    PerformRequest(fd, buffer, nread);
                } else if (nread < 0) {
                    dead_clients.push_back(fd);
                }
            }

            //
            // CLEANUP
            //
            for (i = 0; i < dead_clients.size(); ++i) {
                CloseClient(dead_clients[i]);
            }
        }
    }

    ssize_t SSLServer::SSLRead(ClientConnection& client, char* buffer, size_t len) {
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

    ssize_t SSLServer::SSLWrite(ClientConnection& client, const char* buffer, size_t len) {
        int ret = SSL_write(client.ssl, buffer, len);
        if (ret > 0) {
            return ret;
        }

        return -1;
    }

    void SSLServer::PerformRequest(const int sd, const char* buffer, const ssize_t nread) {
        if (ssl_clients_.find(sd) == ssl_clients_.end()) {
            return;
        }

        ClientConnection& client = ssl_clients_[sd];
        std::string raw_request(buffer, nread);
        Request req(raw_request);
        std::string response = HandleRoute(req);

        SSLWrite(client, response.c_str(), response.size());
    }

    std::string SSLServer::HandleRoute(http::Request& req) {
        Response res(req.is_keep_alive(), static_directory_);

        if (req.mime_type().has_value()) {
            std::string content = ReadFile(static_directory_ + req.path());

            if (!content.empty()) {
                res.SetContentByType(content, req.mime_type().value());
            } else {
                res.SetContent<ContentType::PLAIN_TEXT, Status::not_found>("Not Found");
            }
        } else {
            request_handler handler;
            if (router_.Match(&req, &handler)) {
                handler(req, res);
            } else {
                res.SetContent<ContentType::PLAIN_TEXT, Status::not_found>("Not Found");
            }
        }

        return res.Build();
    }

    void SSLServer::Stop() {
        running_ = false;

        std::vector<int> clients;
        std::unordered_map<int, ClientConnection>::iterator it;
        for (it = ssl_clients_.begin(); it != ssl_clients_.end(); ++it) {
            clients.push_back(it->first);
        }

        size_t i;
        for (i = 0; i < clients.size(); ++i) {
            CloseClient(clients[i]);
        }

        if (sockfd_ != -1) {
            close(sockfd_);
            sockfd_ = -1;
        }

        log.Info("HTTPS server stopped");
    }

}  // namespace http