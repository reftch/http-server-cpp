// server.hpp
#ifndef SERVER_HPP
#define SERVER_HPP

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstring>
#include <functional>
#include <future>
#include <iostream>
#include <map>
// #include <regex>
#include <string>
#include <thread>
#include <vector>

#include "request.hpp"
#include "response.hpp"
#include "router.hpp"
#include "utils.hpp"

namespace http {

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
        server(const std::string& host, const int& port) : port(port), host(host) {
            start_time = std::chrono::high_resolution_clock::now();
        }

        /**
         * Starts the server and begins listening for incoming connections.
         * This method runs in an infinite loop until interrupted.
         *
         * @return 0 on successful start, non-zero on error
         * @return 1 on socket creation error
         * @return 2 on error of multiple connections
         * @return 3 on connection binding error
         * @return 4 on connection listener error
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
        int path(const std::string& method, const std::string& path, request_handler handler);

        // getters
        std::string get_host() const { return host; }
        int get_port() const { return port; }
        bool running() { return running_; }

       private:
        // Private members related to the server state (optional, but good practice)
        const int port;         /**< Port number to listen on */
        const std::string host; /**< Hostname or IP address to bind to */
        int32_t sockfd;         // server file descriptor
        std::vector<int> client_list;
        std::vector<struct pollfd> pollfds;

        // router
        http::router g_router;

        bool running_ = false; /** is server running flag */

        std::chrono::high_resolution_clock::time_point start_time; /**< Time when server started */

        /**
         * Maximum number of connections allowed.
         * This constant defines the maximum capacity of the server's connection handling.
         */
        static constexpr int MAX_CONNS = 96;

        /**
         * Handle an incoming HTTP request on a given connection.
         * Processes data received on the connection and handles the request logic.
         *
         * @return 0 on success, -1 on error or connection close
         */
        void handle_requests();

        std::string handle_route(const std::string& method, const std::string& path);
    };

}  // namespace http

#endif  // SERVER_HPP