// server.cpp
#include "server.hpp"

#include "request.hpp"
#include "response.hpp"

namespace http::server {

    /**
     * Signals the server to shut down, stops the polling loop, and closes all sockets.
     */
    void server::stop() {
        std::cout << "\nserver stopping..." << '\n';

        // set the running flag to false to break the while loop in start()
        running_ = false;

        std::cout << "Client list size " << client_list.size() << '\n';
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

        std::cout << "server stopped successfully." << '\n';
    }

    /**
     * Implementation of the start method
     * Sets up the server socket, binds to the specified address, and enters the main event loop.
     * The server will continuously listen for new connections and handle existing connections.
     *
     * @return 0 on successful start, non-zero on error
     */
    int server::start() {
        start_time = std::chrono::high_resolution_clock::now();

        // initialize the running flag ***
        running_ = true;  // Assume this is a member variable

        // start time
        auto start_time = std::chrono::high_resolution_clock::now();

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

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        std::cout << "server listening on http://" << host << ":" << port << " in " << duration << '\n';

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

            // Using poll for listening to multiple clients
            int activity = poll(pollfds.data(), pollfds.size(), 2000);
            if (activity < 0) {
                std::cerr << "poll error\n";
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
                int pollfd_index = i + 1;  // +1 because server socket is at index 0

                if (pollfd_index < pollfds.size() && (pollfds[pollfd_index].revents & POLLIN)) {
                    int sd = client_list[i];
                    char buffer[4096];
                    ssize_t nread = read(sd, buffer, sizeof(buffer) - 1);

                    if (nread > 0) {
                        std::string raw_request(buffer, nread);
                        // Parse the request line to find method and path
                        request::http_request request = request::parse(raw_request);
                        // handle route
                        std::string body = handle_route(request.method, request.path);
                        if (write(sd, body.c_str(), body.size()) == -1) {
                            perror("error writing response body");
                        }
                    } else if (nread == 0) {
                        // Client disconnected
                        close(sd);
                        // Remove from client list
                        client_list.erase(client_list.begin() + i);
                        continue;  // Don't increment i since we removed an element
                    } else {
                        // Error or would block (non-blocking socket)
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            // std::cerr << "read error: " << strerror(errno) << '\n';
                        }
                        i++;  // Increment if no error
                    }
                } else {
                    i++;  // Increment if no activity
                }
            }
        }
    }

    /**
     * Hanlde routes
     */
    std::string server::handle_route(const std::string& method, const std::string& path) {
        for (const auto& route_info : routes_) {
            std::smatch matches;
            if (std::regex_match(path, matches, route_info.regex_pattern)) {
                // Extract parameters
                std::unordered_map<std::string, std::string> params;
                for (size_t i = 0; i < route_info.param_names.size(); ++i) {
                    if (i + 1 < matches.size()) {
                        params[route_info.param_names[i]] = matches[i + 1].str();
                    }
                }

                std::string body = route_info.handler(path, params);
                return response::get(response::status::ok, response::content_type::JSON, body);
            }
        }

        std::cout << "No handler found for: " << method << " " << path << '\n';
        return response::get(response::status::not_found, response::content_type::PLAIN_TEXT, "Not Found");
    }

    /**
     * Implementation of the required method to register handlers.
     */
    int server::register_handler(const std::string& method, const std::string& path, request_handler handler) {
        // convert path with :id to regex pattern
        std::string regex_pattern = "^" + path + "$";
        std::vector<std::string> param_names;

        // replace :param with regex group
        size_t pos = 0;
        while ((pos = regex_pattern.find(':')) != std::string::npos) {
            // find the end of the parameter name (next slash or end of string)
            size_t end_pos = pos + 1;
            while (end_pos < regex_pattern.length() && regex_pattern[end_pos] != '/' && regex_pattern[end_pos] != '$') {
                end_pos++;
            }

            std::string param_name = regex_pattern.substr(pos + 1, end_pos - pos - 1);
            param_names.push_back(param_name);

            // replace :param_name with regex group
            regex_pattern.replace(pos, end_pos - pos, "([^/]+)");
        }

        // Create route_info structure
        route_info info;
        info.method = method;
        info.pattern = path;
        info.regex_pattern = std::regex(regex_pattern);
        info.param_names = param_names;
        info.handler = handler;

        routes_.push_back(info);

        std::cout << "registered handler for: " << method << " " << path << '\n';
        return 0;
    }

}  // namespace http::server