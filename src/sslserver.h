#ifndef SSL_SERVER_H_
#define SSL_SERVER_H_

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "server.h"

namespace http {

    class SSLServer : public Server {
       public:
        /**
         * Constructor for the SSL server.
         * Initializes SSL context and server configuration.
         *
         * @param host The hostname or IP address to bind to.
         * @param port The port number to listen on.
         * @param cert_file Path to the SSL certificate file.
         * @param key_file Path to the SSL private key file.
         */
        SSLServer(const std::string& host, const int& port, const std::string& cert_file, const std::string& key_file)
            : Server(host, port), cert_file_(cert_file), key_file_(key_file) {
            InitializeSSL();
        }

        /**
         * Destructor for the SSL server.
         * Cleans up SSL context and resources.
         */
        ~SSLServer() { CleanupSSL(); }

        /**
         * Starts the SSL server and begins listening for incoming HTTPS connections.
         * This method runs in an infinite loop until interrupted.
         *
         * @return 0 on successful start, non-zero on error
         * @return 1 on socket creation error
         * @return 2 on error of multiple connections
         * @return 3 on connection binding error
         * @return 4 on connection listener error
         * @return 5 on SSL context initialization error
         * @return 6 on SSL certificate loading error
         */
        int Start() override;

       private:
        std::string cert_file_;
        std::string key_file_;
        SSL_CTX* ssl_ctx_;

        /**
         * Initializes the SSL context for the server.
         * @return true on success, false on failure
         */
        bool InitializeSSL();

        /**
         * Cleans up SSL resources.
         */
        void CleanupSSL();

        /**
         * Creates a new SSL connection for the client.
         * @param client_fd The client socket file descriptor
         * @return SSL* pointer to the new SSL connection or nullptr on error
         */
        SSL* CreateSSLConnection(int client_fd);

        /**
         * Performs an asynchronous HTTPS request handling operation
         * @param sd The socket descriptor for the client connection
         * @param buffer The raw request data received from the client
         * @param nread The number of bytes read from the socket
         * @details This function processes an incoming HTTPS request by:
         *          1. Creating an SSL connection
         *          2. Converting the raw buffer data to a string
         *          3. Parsing the HTTP request to extract method and path
         *          4. Routing the request to the appropriate handler
         *          5. Writing the response back to the client socket through SSL
         */
        void PerformRequest(const int sd, const char* buffer, const ssize_t nread) override;

        /**
         * Handles HTTPS route matching and request processing
         * @param ctx Reference to the HTTP request context containing method, path, and other request data
         * @return String containing the HTTP response body
         * @details This function processes the HTTPS request by:
         *          1. Checking if the request has a specific MIME type
         *          2. If no MIME type is specified, using the global router to match the request
         *          3. If MIME type is specified, treating it as a file request and reading from assets directory
         *          4. Returning appropriate HTTP response based on the processing outcome
         */
        std::string HandleRoute(http::Request& ctx) override;
    };

}  // namespace http

#endif  // SSL_SERVER_H_