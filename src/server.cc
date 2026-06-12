#include "server.h"

namespace http {

    int Server::Start() {
        // open socket
        if ((sockfd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            log.Error("Socket creation failed");
            exit(1);
        }

        // Set SO_REUSEADDR in socket options.
        // For UDP SO_REUSEADDR may mean some problems...
        int optval = 1;
        if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) {
            log.Error("Setting to allow multiple connection failed");
            exit(2);
        }

        // Link socket with address.
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 0.0.0.0
        server_addr.sin_port = htons(port_);
        if (bind(sockfd_, (struct sockaddr*)(&server_addr), sizeof(server_addr)) == -1) {
            log.Error("Bind the server address failed");
            exit(3);
        }

        // Set listen socket to unblocking mode.
        SetNonblockMode(sockfd_);

        // Server is ready to get SOMAXCONN connection requests (128).
        // This is *SHOULD* be enought to call accept and create child process.
        if (listen(sockfd_, SOMAXCONN) == -1) {
            log.Error("Listening the server port failed");
            exit(4);
        }

        // initialize the running flag
        running_ = true;

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);

        log.Info("Server started on http://{}:{} in {} ", host_, port_, duration);

        HandleRequests();

        return 0;
    }

    int Server::SetNonblockMode(int fd) {
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

    void Server::Stop() {
        std::cout << "\n";

        // set the running flag to false to break the while loop in start()
        running_ = false;

        // close all active client connections
        for (auto iter = client_list_.begin(); iter != client_list_.end(); iter++) {
            int sd = *iter;
            if (sd != -1) {
                log.Info("closing client connection FD: {}", sd);
                close(sd);
            }
        }

        // close the listening socket (the main server socket)
        if (sockfd_ != -1) {
            log.Info("Closing listening socket FD: {}", sockfd_);
            close(sockfd_);
        }

        log.Info("Server stopped successfully");
    }

    void Server::HandleRequests() {
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
                log.Error("Error of calling poll");
                exit(5);
            }

            for (int i = 0; i < poll_size; i++) {
                if (descriptors[i].revents & POLLIN) {
                    if (i > 0) {
                        // Slave socket
                        static char buf[BUFFER_SIZE];
                        // ssize_t len = read(descriptors[i].fd, &buf, BUFFER_SIZE - 1);
                        ssize_t len = recv(descriptors[i].fd, buf, BUFFER_SIZE, MSG_NOSIGNAL);

                        if ((len == 0) && (errno != EAGAIN)) {
                            // If we got event TO READ, but actually CANNOT read, this means we should
                            // CLOSE connection. This is how POLL and EPOLL works.
                            shutdown(descriptors[i].fd, SHUT_RDWR);
                            close(descriptors[i].fd);
                            client_list_.erase(descriptors[i].fd);
                        } else if (len > 0) {
                            PerformRequest(descriptors[i].fd, buf, len);
                        }
                    } else {
                        // Master socket
                        struct sockaddr_in client_addr;
                        socklen_t slen = sizeof(client_addr);
                        memset(&client_addr, 0, sizeof(client_addr));
                        int client_fd = accept(sockfd_, (struct sockaddr*)&client_addr, &slen);
                        if (client_fd == -1) {
                            if (errno != EWOULDBLOCK || errno != EAGAIN) {
                                log.Error("Error of calling accept");
                                exit(6);
                            }
                        }
                        // std::cout << "Client = " << inet_ntoa(client_addr.sin_addr) << std::endl;
                        SetNonblockMode(client_fd);
                        client_list_.insert(client_fd);
                    }
                }
            }
        }
    }

    void Server::PerformRequest(const int sd, const char* buffer, const ssize_t nread) {
        std::string raw_request(buffer, nread);
        // Parse the request line to find method and path
        http::Request req(raw_request);
        // Handle route
        std::string body = HandleRoute(req);

        // write response
        const char* ptr = body.c_str();
        ssize_t total_written = 0;
        ssize_t size = body.size();

        while (total_written < size) {
            ssize_t written = write(sd, ptr + total_written, size - total_written);
            // ssize_t written = send(sd, ptr + total_written, size - total_written, MSG_NOSIGNAL);
            if (written == -1) {
                log.Warning("Error writing response body");
                break;
            }
            total_written += written;
        }
    }

    std::string Server::HandleRoute(http::Request& req) {
        Response res(req.is_keep_alive(), static_directory_);

        // check on static resource
        if (req.mime_type().has_value()) {
            auto content = ReadFile(static_directory_ + req.path());
            if (content != "") {
                res.SetContentByType(content, req.mime_type().value());
            }
        } else {
            // Call handler if it exists
            http::request_handler handler;
            if (router_.Match(&req, &handler)) {
                handler(req, res);
            } else {
                res.SetContent<ContentType::PLAIN_TEXT, Status::not_found>("Not Found");
            }
        }

        return res.Build();
    }

}  // namespace http