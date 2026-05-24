#include "server.h"

namespace http {

    void Server::Stop() {
        std::cout << "\n";

        // set the running flag to false to break the while loop in start()
        running_ = false;

        // close all active client connections
        for (size_t i = 0; i < client_list_.size(); ++i) {
            int sd = client_list_[i];
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

    int Server::Start() {
        // open socket
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);  // for tcp connection
        // error handling
        if (sockfd_ <= 0) {
            log.Error("Socket creation failed");
            exit(1);
        }

        // setting serverFd to allow multiple connection
        int opt = 1;
        if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof opt) < 0) {
            log.Error("Setting to allow multiple connection failed");
            exit(2);
        }

        // setting the server address
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port_);
        inet_pton(AF_INET, host_.data(), &serverAddr.sin_addr);

        // binding the server address
        if (bind(sockfd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            log.Error("Bind the server address failed");
            exit(3);
        }

        // listening to the port
        if (listen(sockfd_, kMAX_CONNS) < 0) {
            log.Error("Listening the server port failed");
            exit(4);
        }

        // initialize the running flag
        running_ = true;

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);

        log.Info("Server started on https://{}:{} in {} ", host_, port_, duration);

        HandleRequests();

        return 0;
    }

    void Server::HandleRequests() {
        // Vector to hold pollfd structures
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

            // Add all client sockets
            for (auto sd : client_list_) {
                struct pollfd pfd;
                pfd.fd = sd;
                pfd.events = POLLIN;
                pfd.revents = 0;
                pollfds.push_back(pfd);
            }

            // Using poll for listening to multiple clients with timeout
            int activity = poll(pollfds.data(), pollfds.size(), kCONNECTION_TIMEOUT_SECOND * 1000);
            if (activity < 0) {
                log.Warning("Stop poll for listening");
                continue;
            }

            // Check for new connection on server socket
            if (pollfds[0].revents & POLLIN) {
                // client file descriptor
                auto clientfd = accept(sockfd_, (struct sockaddr*)NULL, NULL);
                if (clientfd < 0) {
                    log.Warning("Accepted error");
                    continue;
                }
                // Set non-blocking mode for client socket
                int flags = fcntl(clientfd, F_GETFL, 0);
                fcntl(clientfd, F_SETFL, flags | O_NONBLOCK);

                // adding new client to list
                client_list_.push_back(clientfd);
            }

            // check for activity on client sockets, process each client socket
            for (size_t i = 0; i < client_list_.size();) {
                size_t pollfd_index = i + 1;  // +1 because server socket is at index 0

                if (pollfd_index < pollfds.size() && (pollfds[pollfd_index].revents & POLLIN)) {
                    int sd = client_list_[i];
                    char buffer[READ_BUFFER_SIZE];
                    ssize_t nread = read(sd, &buffer, sizeof(buffer) - 1);
                    if (nread > 0) {
                        // perform request
                        PerformRequest(sd, buffer, nread);
                        // Launch asynchronously
                        // auto future = std::async(std::launch::async, [&]() { perform_request(sd, buffer, nread); });
                    } else if (nread == 0) {
                        // Client disconnected
                        close(sd);
                        // Remove from client list
                        client_list_.erase(client_list_.begin() + i);
                        continue;  // Don't increment i since we removed an element
                    } else {
                        // Error or would block (non-blocking socket)
                        // if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        // std::cerr << "read error: " << strerror(errno) << '\n';
                        // }
                        i++;
                    }
                } else {
                    i++;  // Increment if no activity
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
        if (write(sd, body.c_str(), body.size()) == -1) {
            log.Warning("Error writing response body");
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