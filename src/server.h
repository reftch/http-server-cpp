#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#ifndef KEEPALIVE_MAX_COUNT
#define KEEPALIVE_MAX_COUNT 100
#endif

#ifndef CONNECTION_TIMEOUT_SECOND
#define CONNECTION_TIMEOUT_SECOND 3
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
#include <openssl/err.h>
#include <openssl/ssl.h>
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
#include <unordered_map>
#include <vector>

#include "logger.h"
#include "request.h"
#include "response.h"
#include "router.h"
#include "utils.h"

namespace http {

    // HTTP Method enum
    enum class HttpMethod { GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS };

    class Server {
       public:
        struct ClientConnection {
            int fd = -1;
            SSL* ssl = nullptr;
            bool handshake_completed = false;
        };

        /**
         * HTTP Constructor
         */
        Server(const std::string& host, const int& port) : host_(host), port_(port) {
            start_time_ = std::chrono::high_resolution_clock::now();
        }

        /**
         * HTTPS Constructor
         */
        Server(const std::string& host, const int& port, const std::string& cert_file, const std::string& key_file)
            : host_(host), port_(port), cert_file_(cert_file), key_file_(key_file), ssl_ctx_(nullptr), is_https_(true) {
            start_time_ = std::chrono::high_resolution_clock::now();
            SSL_library_init();
            SSL_load_error_strings();
            OpenSSL_add_ssl_algorithms();
        }

        /**
         * Destructor
         */
        ~Server() {
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
        int Start();

        /**
         * Stop HTTPS server
         */
        void Stop();

        // Getters
        std::string host() const { return host_; }
        int port() const { return port_; }
        bool is_running() { return running_; }

        template <HttpMethod Method>
        void SetRoute(const std::string& path, request_handler handler) {
            router_.RegisterHandler(std::string(ToString(Method)), path, handler);
        }

        void SetAssetDirectory(const std::string& directory) { static_directory_ = directory; }

       protected:
       private:
        const std::string host_;  // Hostname or IP address to bind to
        const int port_;          // Port number to listen on

        //
        // SSL CONFIG
        //
        std::string cert_file_;
        std::string key_file_;

        SSL_CTX* ssl_ctx_;
        bool is_https_ = false;

        int32_t sockfd_;  // server file descriptor

        //
        // SSL CLIENT STORAGE
        //
        std::unordered_map<int, ClientConnection> ssl_clients_;

        // Time when server started
        std::chrono::high_resolution_clock::time_point start_time_;

        // Maximum number of connections allowed.
        static constexpr int kMAX_CONNS = KEEPALIVE_MAX_COUNT;
        static constexpr int kCONNECTION_TIMEOUT_SECOND = CONNECTION_TIMEOUT_SECOND;

        // logger
        Logger& log = Logger::getInstance();

        bool running_ = false;
        std::string static_directory_ = "./assets";

        // router
        http::Router router_;

        /**
         * HTTPS request handling loop
         */
        void HandleRequests();

        /**
         * Route handling
         */
        std::string HandleRoute(http::Request& req);

        /**
         * Initialize SSL context
         */
        bool InitializeSSL();

        /**
         * Close + cleanup client
         */
        void CloseClient(int fd);

        // std::string HandleRoute(http::Request& req);

        /**
         * Perform HTTPS request
         */
        // void PerformHttpsRequest(const int sd, const char* buffer, const ssize_t nread);

        void PerformRequest(const int sd, const char* buffer, const ssize_t nread);

        /**
         * SSL read wrapper
         */
        ssize_t SSLRead(ClientConnection& client, char* buffer, size_t len);

        /**
         * SSL write wrapper
         */
        ssize_t SSLWrite(ClientConnection& client, const char* buffer, size_t len);

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

#endif  // SSL_SERVER_H_