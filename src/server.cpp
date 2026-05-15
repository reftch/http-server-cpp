// server.cpp
#include "server.hpp"

#include <future>
#include <thread>

#include "request.hpp"
#include "response.hpp"

namespace http {

    void server::stop() {
        std::cout << "\nserver stopping...\n";

        // set the running flag to false to break the while loop in start()
        running_ = false;

        // close all active client connections
        for (size_t i = 0; i < client_list.size(); ++i) {
            int sd = client_list[i];
            if (sd != -1) {
                std::cout << "closing client connection FD: " << sd << '\n';
                close(sd);
            }
        }

        // close the listening socket (the main server socket)
        if (sockfd != -1) {
            std::cout << "closing listening socket FD: " << sockfd << '\n';
            close(sockfd);
        }

        std::cout << "server stopped successfully" << '\n';
    }

    int server::start() {
        // open socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);  // for tcp connection
        // error handling
        if (sockfd <= 0) {
            std::cerr << "socket creation error\n";
            exit(1);
        }

        // setting serverFd to allow multiple connection
        int opt = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof opt) < 0) {
            std::cerr << "setSocketopt error\n";
            exit(2);
        }

        // setting the server address
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, host.data(), &serverAddr.sin_addr);

        // binding the server address
        if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "bind error\n";
            exit(3);
        }

        // listening to the port
        if (listen(sockfd, MAX_CONNS) < 0) {
            std::cerr << "listen error\n";
            exit(4);
        }

        // initialize the running flag
        running_ = true;

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        std::cout << "server started on http://" << host << ":" << port << " in " << duration << '\n';

        handle_requests();

        return 0;
    }

    void server::handle_requests() {
        // Vector to hold pollfd structures
        std::vector<struct pollfd> pollfds;

        while (running_) {
            // Clear and rebuild pollfds vector
            pollfds.clear();

            // Add server socket
            struct pollfd server_pollfd;
            server_pollfd.fd = sockfd;
            server_pollfd.events = POLLIN;
            server_pollfd.revents = 0;
            pollfds.push_back(server_pollfd);

            // Add all client sockets
            for (auto sd : client_list) {
                struct pollfd pfd;
                pfd.fd = sd;
                pfd.events = POLLIN;
                pfd.revents = 0;
                pollfds.push_back(pfd);
            }

            // std::cout << "Client size: " << client_list.size() << '\n';

            // Using poll for listening to multiple clients with timeout
            int activity = poll(pollfds.data(), pollfds.size(), 1000);
            if (activity < 0) {
                std::cerr << "polling stop\n";
                continue;
            }

            // Check for new connection on server socket
            if (pollfds[0].revents & POLLIN) {
                // client file descriptor
                auto clientfd = accept(sockfd, (struct sockaddr*)NULL, NULL);
                if (clientfd < 0) {
                    std::cerr << "accept error\n";
                    continue;
                }
                // Set non-blocking mode for client socket
                int flags = fcntl(clientfd, F_GETFL, 0);
                fcntl(clientfd, F_SETFL, flags | O_NONBLOCK);

                // adding client to list
                client_list.push_back(clientfd);
                // std::cout << "new client connected, fd: " << client->clientfd << std::endl;
            }

            // check for activity on client sockets, process each client socket
            for (size_t i = 0; i < client_list.size();) {
                size_t pollfd_index = i + 1;  // +1 because server socket is at index 0

                if (pollfd_index < pollfds.size() && (pollfds[pollfd_index].revents & POLLIN)) {
                    int sd = client_list[i];
                    char buffer[4096];
                    ssize_t nread = read(sd, &buffer, sizeof(buffer) - 1);
                    if (nread > 0) {
                        // perform request
                        perform_request(sd, buffer, nread);
                        // Launch asynchronously
                        // auto future = std::async(std::launch::async, [&]() { perform_request(sd, buffer, nread); });
                    } else if (nread == 0) {
                        // Client disconnected
                        close(sd);
                        // Remove from client list
                        client_list.erase(client_list.begin() + i);
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

    void server::perform_request(const int sd, const char* buffer, const ssize_t nread) {
        std::string raw_request(buffer, nread);
        // Parse the request line to find method and path
        http::request req(raw_request);
        // request::context ctx = request::parse(raw_request);
        // Handle route
        std::string body = handle_route(req);
        // write response
        if (write(sd, body.c_str(), body.size()) == -1) {
            std::cerr << "error writing response body\n";
        }
    }

    std::string server::handle_route(http::request& req) {
        if (req.getMimeType() == "") {
            http::request_handler handler;

            if (g_router.match(&req, &handler)) {
                // Call handler and generate HTTP‑style response body
                return handler(req);
            }
        } else {
            auto content = read_file("./assets" + req.getPath());
            if (content != "") {
                return response::create(req.getMimeType().c_str(), content);
            }
        }

        return response::create(response::status::not_found, response::content_type::PLAIN_TEXT, "Not Found");
    }

    server& server::path(const std::string& method, const std::string& path, request_handler handler) {
        g_router.register_handler(method, path, handler);
        return *this;
    }

}  // namespace http