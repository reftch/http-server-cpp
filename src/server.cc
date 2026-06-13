#include "server.h"

namespace http {

    int Server::start() {
        // open socket
        if ((sockfd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            log.error("Socket creation failed");
            exit(1);
        }

        // Set SO_REUSEADDR in socket options.
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
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 0.0.0.0
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

        // close the listening socket (the main server socket)
        if (sockfd_ != -1) {
            log.info("Closing listening socket FD: {}", sockfd_);
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
                                exit(6);
                            }
                        }

                        // std::cout << "Client = " << inet_ntoa(client_addr.sin_addr) << std::endl;
                        setNonblockMode(client_fd);
                        client_list_.insert(client_fd);
                    }
                }
            }
        }
    }

    void Server::performRequest(const int sd, const char* buffer, const ssize_t nread) {
        std::string raw_request(buffer, nread);

        if (isWebSocketFrame(raw_request)) {
            // log.info("Received message: {}", raw_request);
            ws::Response res;
            std::vector<uint8_t> byte_data(raw_request.begin(), raw_request.end());

            auto frame = res.parseFrame(byte_data);
            log.info("SocketID {}", sd);
            log.info("Frame payload {}", frame.text_payload);
        } else {
            // Parse the request line to find method and path
            http::Request req(raw_request);

            if (req.headers().contains("Upgrade")) {
                // Handle websockt request
                if (makeWebsocketAccept(sd, req)) {
                    log.info("Path {}, socketID {}", req.path(), sd);
                    http::request_handler handler;
                    if (router_.match(&req, &handler)) {
                        log.info("Found handler {}", req.path());
                        // handler(req, res);
                    }
                }
            } else {
                // Handle route
                std::string body = handleRoute(req);
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
            }
        }
    }

    bool Server::makeWebsocketAccept(const int sd, http::Request& req) {
        auto headers = req.headers();
        std::string_view upgrade_view = headers.at("Upgrade");
        std::string upgrade(upgrade_view);

        if (upgrade == "websocket" && headers.contains("Sec-WebSocket-Key")) {
            std::string_view key_view = headers.at("Sec-WebSocket-Key");
            std::string client_key(key_view);

            log.debug("Websocket request for accepting with client key {}", client_key);

            // Base64 encode the hash
            const std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
            std::string accept_key = base64_encode(sha1(client_key + magic));

            std::string response =
                "HTTP/1.1 101 Switching Protocols\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                "Sec-WebSocket-Accept: " +
                accept_key + "\r\n\r\n";

            log.debug("Websocket accepted with key {}", accept_key);

            ssize_t written = send(sd, response.data(), response.size(), MSG_NOSIGNAL);
            if (written == -1) {
                log.error("Error writing response body: {}", strerror(errno));
                return false;
            }
            return true;
        }

        return false;
    }

    std::string Server::handleRoute(http::Request& req) {
        Response res(req.isKeepAlive(), static_directory_);

        // check on static resource
        if (req.mimeType().has_value()) {
            handleStaticResource(req, res);
        } else {
            // Call handler if it exists
            http::request_handler handler;
            if (router_.match(&req, &handler)) {
                handler(req, res);
            } else {
                res.setContent<ContentType::PLAIN_TEXT, Status::not_found>("Not Found");
            }
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

                auto last_modified = fileMtimeToHttpDate(file_stat.st_mtime);
                if (!last_modified.empty()) {
                    res.setHeader("Last-Modified", last_modified);
                }

                auto etag = computeEtag(static_cast<size_t>(file_stat.st_mtime), file_stat.st_size);
                if (!etag.empty()) {
                    res.setHeader("ETag", etag);
                }

                // set content
                res.setContent(content);
            }
        } else {
            res.setContent<ContentType::PLAIN_TEXT, Status::not_found>("Not Found");
        }
    }

}  // namespace http