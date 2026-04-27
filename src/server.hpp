// server.hpp
#ifndef SERVER_HPP
#define SERVER_HPP

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <csignal>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <thread>
#include <vector>

#include "request.hpp"
#include "response.hpp"
#include "utils.hpp"

namespace http::server {

    /**
     * Context structure for server operations.
     * Stores file descriptor information for each connection.
     */
    struct Ctx {
        int fd;
        Ctx() : fd(-1) {}
    };

    // Define the type for our request handler function.
    // The handler now returns the response body (string).
    // It takes the file descriptor (fd) and the context index (i) for interaction.
    using response_body = std::string;
    using request_handler = std::function<response_body(const std::string& path,
                                                        const std::unordered_map<std::string, std::string>& params)>;

    /**
     * Represents an HTTP server instance.
     * Provides functionality to create, configure, and run an HTTP server.
     */
    class server {
       public:
        /**
         * Constructor for the server.
         * Initializes server configuration but does not start listening.
         *
         * @param host The hostname or IP address to bind to.
         * @param port The port number to listen on.
         */
        server(const std::string& host, const std::string& port);

        // You would typically add other methods here (e.g., start(), handleRequest(), etc.)

        /**
         * Starts the server and begins listening for incoming connections.
         * This method runs in an infinite loop until interrupted.
         *
         * @return 0 on successful start, non-zero on error
         */
        int start();

        /**
         * Signals the server to shut down, stops the polling loop, and closes all sockets.
         */
        void stop();

        /**
         * Registers a handler function for a specific HTTP method and path.
         *
         * @param method The HTTP method (e.g., "GET", "POST").
         * @param path The URL path (e.g., "/index.html").
         * @param handler The function to call when this endpoint is hit.
         * @return 0 on success, -1 if the route is already registered or invalid input.
         */
        int register_handler(const std::string& method, const std::string& path, request_handler handler);

       private:
        // Private members related to the server state (optional, but good practice)
        std::string host_;     /**< Hostname or IP address to bind to */
        std::string port_;     /**< Port number to listen on */
        bool running_ = false; /** is server running flag */

        std::chrono::high_resolution_clock::time_point start_time; /**< Time when server started */

        int sockfd; /**< File descriptor for the listening socket */

        // Structure to hold route information
        struct route_info {
            std::string pattern;
            std::regex regex_pattern;
            std::vector<std::string> param_names;
            request_handler handler;
        };

        // map for routes
        std::vector<route_info> routes_;

        /**
         * Maximum number of connections allowed.
         * This constant defines the maximum capacity of the server's connection handling.
         */
        static constexpr int MAX_CONNS = 96;

        std::vector<Ctx> ctxs;           /**< Vector of connection contexts */
        std::vector<int> fd_to_idx;      /**< Mapping from file descriptor to index in ctxs */
        std::vector<struct pollfd> pfds; /**< Vector of pollfd structures for polling */

        /**
         * Set socket to non-blocking mode.
         * Makes the socket operations non-blocking to enable efficient polling.
         *
         * @param fd Socket file descriptor
         * @return 0 on success, -1 on error
         */
        int set_nonblocking(int fd);

        /**
         * Handle new incoming connection.
         * Accepts a new connection and adds it to the connection tracking system.
         *
         * @param sockfd Listening socket file descriptor
         * @return 0 on success, -1 on error
         */
        int accept_connection(int sockfd);

        /**
         * Handle an incoming HTTP request on a given connection.
         * Processes data received on the connection and handles the request logic.
         *
         * @param fd File descriptor of the connection
         * @param i Index in the internal context vector
         * @return 0 on success, -1 on error or connection close
         */
        int handle_request(int fd, int i);

        std::string handle_route(const std::string& method, const std::string& path);
    };

}  // namespace http::server

#endif  // SERVER_HPP