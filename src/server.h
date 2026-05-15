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
#include <string>
#include <thread>
#include <vector>

#include "header.h"
#include "request.h"
#include "response.h"
#include "router.h"
#include "utils.h"

namespace http {

    /**
     * Represents an HTTP server instance.
     * Provides functionality to create, configure, and run an HTTP server.
     */
    class Server {
       public:
        /**
         * Constructor for the server.
         * Initializes server configuration but does not start listening.
         *
         * @param host The hostname or IP address to bind to.
         * @param port The port number to listen on.
         */
        Server(const std::string& host, const int& port) : port(port), host(host) {
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
         * @return Reference to the server instance for method chaining
         */
        Server& set_path(const std::string& method, const std::string& path, request_handler handler);

        // Getters
        std::string get_host() const {
            return host;
        }
        int get_port() const {
            return port;
        }
        bool is_running() {
            return running_;
        }

       private:
        const int port;                // Port number to listen on
        const std::string host;        // Hostname or IP address to bind to
        int32_t sockfd;                // server file descriptor
        std::vector<int> client_list;  // client list

        // router
        http::router g_router;

        // Is server running flag
        bool running_ = false;

        // Time when server started
        std::chrono::high_resolution_clock::time_point start_time;

        // Maximum number of connections allowed.
        static constexpr int MAX_CONNS = 96;

        /**
         * Performs an asynchronous HTTP request handling operation
         * @param sd The socket descriptor for the client connection
         * @param buffer The raw request data received from the client
         * @param nread The number of bytes read from the socket
         * @details This function processes an incoming HTTP request by:
         *          1. Converting the raw buffer data to a string
         *          2. Parsing the HTTP request to extract method and path
         *          3. Routing the request to the appropriate handler
         *          4. Writing the response back to the client socket
         * @note This function is typically called asynchronously to handle multiple concurrent connections
         */
        void perform_request(const int sd, const char* buffer, const ssize_t nread);

        /**
         * Main event loop for handling incoming HTTP requests
         * @details This function implements a non-blocking I/O event loop using poll() to monitor
         *          both the server socket for new connections and client sockets for incoming data.
         *          It continuously monitors multiple file descriptors and processes events as they occur.
         * @note This function runs in an infinite loop until the server's running_ flag is set to false
         */
        void handle_requests();

        /**
         * Handles HTTP route matching and request processing
         * @param ctx Reference to the HTTP request context containing method, path, and other request data
         * @return String containing the HTTP response body
         * @details This function processes the HTTP request by:
         *          1. Checking if the request has a specific MIME type
         *          2. If no MIME type is specified, using the global router to match the request
         *          3. If MIME type is specified, treating it as a file request and reading from assets directory
         *          4. Returning appropriate HTTP response based on the processing outcome
         * @note This function is typically called from perform_request() to generate responses for client requests
         */
        std::string handle_route(http::Request& ctx);
    };

}  // namespace http

#endif  // SERVER_HPP