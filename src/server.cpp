// server.cpp
#include "server.hpp"

#include "request.hpp"
#include "response.hpp"

namespace http::server {

    /**
     * Constructor implementation
     * Initializes the server with the specified host and port configuration.
     * Does not establish any network connections or start listening.
     *
     * @param host The hostname or IP address to bind to
     * @param port The port number to listen on
     */
    server::server(const std::string& host, const std::string& port) : host_(host), port_(port) {
        start_time = std::chrono::high_resolution_clock::now();

        // Initialize connection tracking structures
        ctxs.reserve(MAX_CONNS);
        for (int i = 0; i < MAX_CONNS; ++i) {
            ctxs[i].fd = -1;
        }

        // fd -> index mapping (simple for FDs < MAX_CONNS)
        fd_to_idx.reserve(MAX_CONNS);
        fd_to_idx.resize(MAX_CONNS, -1);

        // Build pollfd array
        pfds.reserve(MAX_CONNS);
    }

    /**
     * Signals the server to shut down, stops the polling loop, and closes all sockets.
     */
    void server::stop() {
        std::cout << "\nserver stopping..." << '\n';

        // set the running flag to false to break the while loop in start()
        running_ = false;

        // close all active client connections
        for (int i = 0; i < MAX_CONNS; ++i) {
            if (ctxs[i].fd != -1) {
                std::cout << "closing client connection FD: " << ctxs[i].fd << '\n';
                close(ctxs[i].fd);
                fd_to_idx[ctxs[i].fd] = -1;
                ctxs[i].fd = -1;
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
        // initialize the running flag ***
        running_ = true;  // Assume this is a member variable

        // create server socket
        // AF_INET: IPv4 protocol, SOCK_STREAM: TCP socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket");
            return 1;
        }

        // set sockek options
        // SO_REUSEADDR: allow local address reuse
        int one = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

        // define server address
        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;              // INADDR_ANY: Accept connections on any IP
        addr.sin_port = htons(std::stoi(this->port_));  // htons(): Converts port to network byte order

        // bind Socket to Address and listen for incoming Connections
        if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0 || listen(sockfd, 100) < 0) {
            perror("bind/listen");
            close(sockfd);
            return 1;
        }

        // fd -> index mapping (simple for FDs < MAX_CONNS)
        fd_to_idx[sockfd] = sockfd;  // Use FD as index

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        std::cout << "server listening on http://" << this->host_ << ":" << this->port_ << " in " << duration << '\n';

        int max_fd = sockfd;

        while (running_) {
            // clear pollfd array from active FDs
            pfds.clear();

            // Add listening socket
            struct pollfd pfd_sock = {sockfd, POLLIN, 0};
            pfds.push_back(pfd_sock);

            // Add client sockets
            for (int i = 0; i < MAX_CONNS; ++i) {
                if (ctxs[i].fd != -1) {
                    struct pollfd pfd = {ctxs[i].fd, POLLIN, 0};
                    pfds.push_back(pfd);
                    if (ctxs[i].fd > max_fd) max_fd = ctxs[i].fd;
                }
            }

            // Wait for events (infinite timeout)
            int n = poll(pfds.data(), pfds.size(), 2000);
            if (n < 0) {
                perror("poll");
                break;
            }

            if (n == 0) continue;

            // std::cout << "Size: " << pfds.size() << '\n';

            // Process events
            for (size_t j = 0; j < pfds.size(); ++j) {
                if (pfds[j].revents == 0) continue;

                int fd = pfds[j].fd;

                if (fd == sockfd) {
                    // New connection
                    accept_connection(sockfd);
                } else {
                    // Existing client
                    handle_request(fd, j);
                }
            }
        }

        // close socket
        close(sockfd);
        return 0;
    }

    /**
     * Handle new incoming connection.
     * Accepts a new connection from the listening socket and sets it up for non-blocking I/O.
     * Adds the new connection to the connection tracking system.
     *
     * @param sockfd Listening socket file descriptor
     * @return 0 on success, -1 on error
     */
    int server::accept_connection(int sockfd) {
        // accept client connection
        struct sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
        if (connfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return -1;
            perror("accept");
            return -1;
        }

        if (connfd >= MAX_CONNS) {
            std::cout << "too many connections" << '\n';
            close(connfd);
            return -1;
        }

        // set socket to non-blocking mode
        if (set_nonblocking(connfd) < 0) {
            close(connfd);
            return -1;
        }

        fd_to_idx[connfd] = connfd;
        ctxs[connfd].fd = connfd;

        return 0;
    }

    /**
     * Handle an incoming HTTP request on a given connection.
     * Processes data received on the connection and handles the request logic.
     *
     * @param fd File descriptor of the connection
     * @param i Index in the internal context vector
     * @return 0 on success, -1 on error or connection close
     */
    int server::handle_request(int fd, int i) {
        int idx = fd_to_idx[fd];
        if (idx < 0 || idx >= MAX_CONNS || ctxs[idx].fd != fd) return -1;

        Ctx& ctx = ctxs[idx];

        if (pfds[i].revents & POLLHUP || pfds[i].revents & POLLERR) {
            close(fd);
            fd_to_idx[fd] = -1;
            ctx.fd = -1;
            return -1;
        }

        if (pfds[i].revents & POLLIN) {
            char buffer[4096];
            ssize_t nread = read(fd, buffer, sizeof(buffer));
            if (nread > 0) {
                std::string raw_request(buffer, nread);

                // Parse the request line to find method and path
                request::http_request request = request::parse(raw_request);

                // handle route
                std::string body = handle_route(request.method, request.path);
                if (write(fd, body.c_str(), body.size()) == -1) {
                    perror("Error writing response body");
                }
            } else if (nread == 0) {
                // Client closed connection
                close(fd);
                fd_to_idx[fd] = -1;
                ctx.fd = -1;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // More data coming
                } else {
                    // Handle read error
                    std::cout << "read error on fd " << fd << "\n";
                    close(fd);
                    fd_to_idx[fd] = -1;
                    ctx.fd = -1;
                }
            }
        }

        return 0;
    }

    /**
     * Hanlde routes
     *
     *
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
     * Set socket to non-blocking mode.
     * Configures the socket to operate in non-blocking mode, which is essential for efficient
     * polling-based I/O multiplexing.
     *
     * @param fd Socket file descriptor
     * @return 0 on success, -1 on error
     */
    int server::set_nonblocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) return -1;
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) return -1;
        return 0;
    }

    /**
     * Implementation of the required method to register handlers.
     */
    int server::register_handler(const std::string& method, const std::string& path, request_handler handler) {
        // Convert path with :id to regex pattern
        std::string regex_pattern = "^" + path + "$";
        std::vector<std::string> param_names;

        // Replace :param with regex group
        size_t pos = 0;
        while ((pos = regex_pattern.find(':')) != std::string::npos) {
            // Find the end of the parameter name (next slash or end of string)
            size_t end_pos = pos + 1;
            while (end_pos < regex_pattern.length() && regex_pattern[end_pos] != '/' && regex_pattern[end_pos] != '$') {
                end_pos++;
            }

            std::string param_name = regex_pattern.substr(pos + 1, end_pos - pos - 1);
            param_names.push_back(param_name);

            // Replace :param_name with regex group
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

        std::cout << "Registered handler for: " << method << " " << path << '\n';
        return 0;
    }

}  // namespace http::server