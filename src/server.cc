#include "server.h"

namespace http {

    int Server::start() {
        // open socket
        if ((sockfd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            log.error("Socket creation failed");
            exit(1);
        }

        // Set SO_REUSEADDR in socket options
        // For UDP SO_REUSEADDR may mean some problems...
        int optval = 1;
        if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) {
            log.error("Setting to allow multiple connection failed");
            exit(2);
        }

        // Link socket with address.
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(host_.c_str());
        server_addr.sin_port = htons(port_);
        if (bind(sockfd_, (struct sockaddr*)(&server_addr), sizeof(server_addr)) == -1) {
            log.error("Bind the server address failed");
            exit(3);
        }

        // Set listen socket to unblocking mode.
        setNonblockMode(sockfd_);

        // Server is ready to get SOMAXCONN connection requests (128).
        // This is *SHOULD* be enought to call accept and create child process.
        if (listen(sockfd_, SOMAXCONN) == -1) {
            log.error("Listening the server port failed");
            exit(4);
        }

        // initialize the running flag
        running_ = true;

        auto end_time = std::chrono::high_resolution_clock::now();
        // log.info("Time : {}", end_time);
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);

        log.info("Server started on http://{}:{} in {} ", host_, port_, duration);

        handleRequests();

        return 0;
    }

    int Server::setNonblockMode(int fd) {
        int flags;
#if defined(O_NONBLOCK)
        if (-1 == (flags = fcntl(fd, F_GETFL, 0))) {
            flags = 0;
        }

        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
        flags = 1;
        return ioctl(fd, FIONBIO, &flags);
#endif
    }

    void Server::stop() {
        std::cout << "\n";

        // set the running flag to false to break the while loop in start()
        running_ = false;

        // close all active client connections
        for (auto iter = client_list_.begin(); iter != client_list_.end(); iter++) {
            int sd = *iter;
            if (sd != -1) {
                log.info("closing client connection FD: {}", sd);
                close(sd);
            }
        }

        // close all websocket connections
        for (auto iter = wsRoutes.begin(); iter != wsRoutes.end(); iter++) {
            auto wsRoute = *iter;
            if (wsRoute.sockfd != -1) {
                log.info("closing websocket connection FD: {}", wsRoute.sockfd);
                close(wsRoute.sockfd);
            }
        }

        // close the listening socket (the main server socket)
        if (sockfd_ != -1) {
            log.info("closing master connection socket FD: {}", sockfd_);
            close(sockfd_);
        }

        log.info("Server stopped successfully");
    }

    void Server::handleRequests() {
        // Put listen socket at the beginning of all polling descriptors.
        struct pollfd descriptors[POLL_SIZE];
        descriptors[0].fd = sockfd_;
        descriptors[0].events = POLLIN;

        while (running_) {
            int index = 1;
            for (auto iter = client_list_.begin(); iter != client_list_.end(); iter++) {
                descriptors[index].fd = *iter;
                descriptors[index].events = POLLIN;
                ++index;
            }

            // Plus 1 for the listen socket.
            int poll_size = 1 + client_list_.size();

            int ready_fd = poll(descriptors, poll_size, POLL_TIMEOUT);
            if (ready_fd == -1) {
                log.error("Error of calling poll");
                exit(5);
            }

            for (int i = 0; i < poll_size; i++) {
                if (descriptors[i].revents & POLLIN) {
                    if (i > 0) {
                        // Slave socket
                        static char buf[BUFFER_SIZE];
                        ssize_t len = recv(descriptors[i].fd, buf, BUFFER_SIZE, MSG_NOSIGNAL);

                        if ((len == 0) && (errno != EAGAIN)) {
                            // If we got event TO READ, but actually CANNOT read, this means we should
                            // CLOSE connection. This is how POLL and EPOLL works.
                            shutdown(descriptors[i].fd, SHUT_RDWR);
                            close(descriptors[i].fd);
                            client_list_.erase(descriptors[i].fd);
                        } else if (len > 0) {
                            performRequest(descriptors[i].fd, buf, len);
                        }
                    } else {
                        // Master socket
                        struct sockaddr_in client_addr;
                        socklen_t slen = sizeof(client_addr);
                        memset(&client_addr, 0, sizeof(client_addr));
                        int client_fd = accept(sockfd_, (struct sockaddr*)&client_addr, &slen);
                        if (client_fd == -1) {
                            if (errno != EWOULDBLOCK || errno != EAGAIN) {
                                log.error("Error of calling accept");
                                continue;
                            }
                        }

                        setNonblockMode(client_fd);
                        client_list_.insert(client_fd);
                        log.debug("Open socket {}", client_fd);
                    }
                }
            }

            validateSockets();
        }
    }

    void Server::validateSockets() {
        std::vector<int> sockets_to_remove;
        for (auto iter = client_list_.begin(); iter != client_list_.end(); iter++) {
            int sd = *iter;
            if (!utils::isSocketAlive(sd)) {
                log.debug("Socket {} is closed, it will removed from pool", sd);
                sockets_to_remove.push_back(sd);
            }
        }

        for (int sd : sockets_to_remove) {
            client_list_.erase(sd);
        }
    }

    void Server::performRequest(const int sd, const char* buffer, const ssize_t nread) {
        std::string raw_request(buffer, nread);
        // Parse the request line to find method and path
        http::Request req(raw_request);

        // is websocket requests
        if (isWebSocketFrame(raw_request)) {
            // log.info("Websocket message ");
            WebSocket ws(sd, raw_request);
            auto handler = getWsHandlerBySocketId(sd);
            if (handler.has_value()) {
                handler.value()(req, ws);
            }
            return;
        }

        if (processWebsocketHandshake(sd, req)) {
            // update route with socket id
            updateWsRoute(req.path(), sd);
            return;
        }

        // handle request
        std::string body = handleRoute(req);
        // send server response
        sendResponse(sd, body);
    }

    bool Server::sendResponse(const int sd, std::string& body) {
        // write response
        const char* ptr = body.c_str();
        ssize_t total_written = 0;
        ssize_t size = body.size();

        while (total_written < size) {
            ssize_t written = send(sd, ptr + total_written, size - total_written, MSG_NOSIGNAL);
            if (written == -1) {
                // Check if it's a temporary error
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // This is expected in non-blocking mode - just continue
                    // But we should add a timeout mechanism for large files
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                } else {
                    log.warning("Error writing response body: {}", strerror(errno));
                    break;
                }
            }
            total_written += written;
        }

        if (total_written == -1) {
            log.error("Error writing response body: {}", strerror(errno));
            return false;
        }

        return true;
    }

    std::string Server::handleRoute(http::Request& req) {
        Response res(req.isKeepAlive(), static_directory_);

        http::request_handler handler;
        if (router_.match(&req, &handler)) {
            handler(req, res);
        } else if (req.method() == "GET" || req.method() == "HEAD") {
            handleStaticResource(req, res);
        }

        return res.build();
    }

    void Server::handleStaticResource(http::Request& req, http::Response& res) {
        std::string path = static_directory_ + req.path();
        struct stat file_stat;

        if (stat(path.c_str(), &file_stat) == 0) {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (file) {
                // Get file size and read
                size_t size = file.tellg();
                file.seekg(0, std::ios::beg);

                std::string content(size, '\0');
                file.read(content.data(), size);
                file.close();

                // set basic headers
                res.setHeader("Content-Type", req.mimeType().value());
                res.setHeader("Cache-Control", "public, max-age=3600");
                res.setHeader("Content-Length", std::to_string(size));

                auto last_modified = ::utils::fileMtimeToHttpDate(file_stat.st_mtime);
                if (!last_modified.empty()) {
                    res.setHeader("Last-Modified", last_modified);
                }

                auto etag = ::utils::computeEtag(static_cast<size_t>(file_stat.st_mtime), file_stat.st_size);
                if (!etag.empty()) {
                    res.setHeader("ETag", etag);
                }

                // set content
                res << content;
            }
        } else {
            res.setContent<ContentType::PLAIN_TEXT, Status::not_found>("Not Found");
        }
    }

    bool Server::processWebsocketHandshake(const int sd, http::Request& req) {
        const auto& headers = req.headers();

        // Fast path: check both headers in one go with minimal string operations
        auto upgrade_it = headers.find("Upgrade");
        if (upgrade_it == headers.end()) {
            return false;
        }

        auto connection_it = headers.find("Connection");
        if (connection_it == headers.end() && connection_it->second != "Upgrade") {
            return false;
        }

        // Direct comparison without creating temporary strings
        const std::string_view& upgrade_value = upgrade_it->second;
        if (upgrade_value.size() != 9 || (upgrade_value[0] != 'w' && upgrade_value[0] != 'W') ||
            upgrade_value.substr(0, 9) != "websocket") {
            return false;
        }

        // Check Sec-WebSocket-Key
        auto key_it = headers.find("Sec-WebSocket-Key");
        if (key_it == headers.end()) {
            return false;
        }

        const std::string_view& client_key = key_it->second;
        if (client_key.empty()) {
            return false;
        }

        // Check Sec-WebSocket-Key is a valid base64-encoded 16-byte value (24 chars)
        // RFC 6455 Section 4.2.1
        if (client_key.size() != 24 || client_key[22] != '=' || client_key[23] != '=') {
            return false;
        }

        log.debug("Websocket request for accepting with client key {}",
                  std::string(client_key));  // Only convert to string for logging

        // Base64 encode the hash
        const std::string_view magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        std::string accept_key = ::utils::base64_encode(::utils::sha1(std::string(client_key) + std::string(magic)));

        std::string response;
        response =
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: ";

        response += accept_key;
        response += "\r\n\r\n";

        log.debug("Websocket accepted with key {}", accept_key);

        return sendResponse(sd, response);
    }

    // void Server::validateSockets() {
    //     std::vector<int> sockets_to_remove;
    //     for (auto iter = client_list_.begin(); iter != client_list_.end(); iter++) {
    //         int sd = *iter;
    //         if (!utils::isSocketAlive(sd)) {
    //             log.debug("Socket {} is closed, it will removed from pool", sd);
    //             sockets_to_remove.push_back(sd);
    //         }
    //     }

    //     for (int sd : sockets_to_remove) {
    //         client_list_.erase(sd);
    //     }
    // }

}  // namespace http