#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#ifndef KEEPALIVE_MAX_COUNT
#define KEEPALIVE_MAX_COUNT 100
#endif

#ifndef CONNECTION_TIMEOUT_SECOND
#define CONNECTION_TIMEOUT_SECOND 300
#endif

#ifndef CLIENT_TIMEOUT_SECONDS
#define CLIENT_TIMEOUT_SECONDS 10
#endif

#ifndef READ_BUFFER_SIZE
#define READ_BUFFER_SIZE 4096
#endif

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

// #include "client.h"
#include "logger.h"
#include "request.h"
#include "response.h"
#include "router.h"
#include "utils.h"

namespace http {

    // HTTP Method enum
    enum class HttpMethod { GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS };

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
        Server(const std::string& host, const int& port) : port_(port), host_(host) {
            start_time_ = std::chrono::high_resolution_clock::now();
        }

        virtual ~Server() = default;

        // Getters
        std::string host() const { return host_; }
        int port() const { return port_; }
        bool is_running() { return running_; }

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
        virtual int Start();

        /**
         * Signals the server to shut down, stops the polling loop, and closes all sockets.
         */
        virtual void Stop();

        template <HttpMethod Method>
        void SetRoute(const std::string& path, request_handler handler) {
            router_.RegisterHandler(std::string(ToString(Method)), path, handler);
        }

        void SetAssetDirectory(const std::string& directory) { static_directory_ = directory; }

       protected:
        // logger
        Logger& log = Logger::getInstance();
        std::string static_directory_ = "./assets";
        // router
        http::Router router_;

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
        virtual void PerformRequest(const int sd, const char* buffer, const ssize_t nread);

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
        virtual std::string HandleRoute(http::Request& ctx);

        // Is server running flag
        bool running_ = false;
        std::vector<int> client_list_;  // client list
        int32_t sockfd_;                // server file descriptor
        const int port_;                // Port number to listen on
        const std::string host_;        // Hostname or IP address to bind to

        // Maximum number of connections allowed
        static constexpr int kMAX_CONNS = KEEPALIVE_MAX_COUNT;
        static constexpr int kCONNECTION_TIMEOUT_SECOND = CONNECTION_TIMEOUT_SECOND;

        // Time when server started
        std::chrono::high_resolution_clock::time_point start_time_;

        /**
         * Main event loop for handling incoming HTTP requests
         * @details This function implements a non-blocking I/O event loop using poll() to monitor
         *          both the server socket for new connections and client sockets for incoming data.
         *          It continuously monitors multiple file descriptors and processes events as they occur.
         * @note This function runs in an infinite loop until the server's running_ flag is set to false
         */
        virtual void HandleRequests();

        constexpr std::string_view ToString(HttpMethod method) {
            switch (method) {
                case HttpMethod::GET:
                    return "GET";
                case HttpMethod::POST:
                    return "POST";
                case HttpMethod::PUT:
                    return "PUT";
                case HttpMethod::DELETE:
                    return "DELETE";
                case HttpMethod::PATCH:
                    return "PATCH";
                case HttpMethod::HEAD:
                    return "HEAD";
                case HttpMethod::OPTIONS:
                    return "OPTIONS";
            }
            return "OPTIONS";  // or handle as unreachable
        }
    };

}  // namespace http

#endif  // SERVER_HPP