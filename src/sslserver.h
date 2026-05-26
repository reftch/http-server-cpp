#ifndef SSL_SERVER_H_
#define SSL_SERVER_H_

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <unordered_map>

#include "server.h"

namespace http {

    class SSLServer : public Server {
       public:
        struct ClientConnection {
            int fd = -1;
            SSL* ssl = nullptr;
            bool handshake_completed = false;
        };

        /**
         * Constructor
         */
        SSLServer(const std::string& host, const int& port, const std::string& cert_file, const std::string& key_file)
            : Server(host, port), cert_file_(cert_file), key_file_(key_file), ssl_ctx_(nullptr) {
            SSL_library_init();
            SSL_load_error_strings();
            OpenSSL_add_ssl_algorithms();
        }

        /**
         * Destructor
         */
        ~SSLServer() override {
            Stop();
            if (ssl_ctx_) {
                SSL_CTX_free(ssl_ctx_);
                ssl_ctx_ = nullptr;
            }

            EVP_cleanup();
        }

        /**
         * Start HTTPS server
         */
        int Start() override;

        /**
         * Stop HTTPS server
         */
        void Stop() override;

       protected:
        /**
         * HTTPS request handling loop
         */
        void HandleRequests() override;

        /**
         * Route handling
         */
        std::string HandleRoute(http::Request& req) override;

       private:
        //
        // SSL CONFIG
        //
        std::string cert_file_;
        std::string key_file_;

        SSL_CTX* ssl_ctx_;

        //
        // SSL CLIENT STORAGE
        //
        std::unordered_map<int, ClientConnection> ssl_clients_;

        /**
         * Initialize SSL context
         */
        bool InitializeSSL();

        /**
         * Close + cleanup client
         */
        void CloseClient(int fd);

        /**
         * Perform HTTPS request
         */
        void PerformRequest(const int sd, const char* buffer, const ssize_t nread) override;

        /**
         * SSL read wrapper
         */
        ssize_t SSLRead(ClientConnection& client, char* buffer, size_t len);

        /**
         * SSL write wrapper
         */
        ssize_t SSLWrite(ClientConnection& client, const char* buffer, size_t len);
    };

}  // namespace http

#endif  // SSL_SERVER_H_