#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>

namespace http::server
{
    /**
     * Represents an HTTP server instance.
     */
    class server
    {
    public:
        /**
         * Constructor for the server.
         *
         * @param host The hostname or IP address to bind to.
         * @param port The port number to listen on.
         */
        server(const std::string &host, const std::string &port);

        // You would typically add other methods here (e.g., start(), handleRequest(), etc.)

        void start();
    private:
        // Private members related to the server state (optional, but good practice)
        std::string host_;
        std::string port_;
    };

} // namespace http::server

#endif // SERVER_HPP