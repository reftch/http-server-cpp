#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>

namespace http::server {
/**
 * Represents an HTTP server instance.
 */
class server {
   public:
    /**
     * Constructor for the server.
     *
     * @param host The hostname or IP address to bind to.
     * @param port The port number to listen on.
     */
    server(const std::string& host, const std::string& port);

    // You would typically add other methods here (e.g., start(), handleRequest(), etc.)

    int start();

   private:
    // Private members related to the server state (optional, but good practice)
    std::string host_;
    std::string port_;

    int sockfd;

    /**
     * Maximum number of connections allowed.
     */
    const int MAX_CONNS = 65536;

    /**
     * Context structure for server operations.
     */
    struct Ctx {
        int fd;
        Ctx() : fd(-1) {}
    };

    /**
     * Set socket to non-blocking mode.
     *
     * @param fd Socket file descriptor
     * @return 0 on success, -1 on error
     */
    int setNonblocking(int fd);
};

}  // namespace http::server

#endif  // SERVER_HPP