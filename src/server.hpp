// server.hpp
#ifndef SERVER_HPP
#define SERVER_HPP

#include <poll.h>

#include <chrono>
#include <string>
#include <vector>

namespace http::server {

/**
 * Context structure for server operations.
 * Stores file descriptor information for each connection.
 */
struct Ctx {
    int fd;
    Ctx() : fd(-1) {}
};

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

   private:
    // Private members related to the server state (optional, but good practice)
    std::string host_; /**< Hostname or IP address to bind to */
    std::string port_; /**< Port number to listen on */

    std::chrono::high_resolution_clock::time_point startTime; /**< Time when server started */

    int sockfd; /**< File descriptor for the listening socket */

    /**
     * Maximum number of connections allowed.
     * This constant defines the maximum capacity of the server's connection handling.
     */
    static constexpr int MAX_CONNS = 1024;

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
    int setNonblocking(int fd);

    /**
     * Handle new incoming connection.
     * Accepts a new connection and adds it to the connection tracking system.
     *
     * @param sockfd Listening socket file descriptor
     * @return 0 on success, -1 on error
     */
    int handleNewConnection(int sockfd);

    /**
     * Handle an incoming HTTP request on a given connection.
     * Processes data received on the connection and handles the request logic.
     *
     * @param fd File descriptor of the connection
     * @param i Index in the internal context vector
     * @return 0 on success, -1 on error or connection close
     */
    int handleRequest(int fd, int i);
};

}  // namespace http::server

#endif  // SERVER_HPP