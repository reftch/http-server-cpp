#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <charconv>
#include <cstring>
#include <expected>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

// SSL include
#ifdef HTTP_OPENSSL_SUPPORT
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

#include "logger.h"
#include "response.h"

namespace http {

    // Forward declarations
    class HttpClient;
    class HttpsClient;
    class ClientFactory;

    enum class Protocol { HTTP, HTTPS };

    /**
     * @brief Base class for HTTP clients providing common functionality
     *
     * This class defines the interface for HTTP clients and provides
     * shared functionality like response parsing and socket management.
     */
    class BaseClient {
       public:
        virtual ~BaseClient() = default;

        /**
         * @brief Send a GET request to the specified path
         * @param path The URL path to request
         * @return Expected Response object or error string
         */
        std::expected<Response, std::string> get(const std::string& path);

        /**
         * @brief Send a POST request with body data
         * @param path The URL path to request
         * @param body The request body content
         * @return Expected Response object or error string
         */
        std::expected<Response, std::string> post(const std::string& path, const std::string& body);

        /**
         * @brief Send a PUT request with body data
         * @param path The URL path to request
         * @param body The request body content
         * @return Expected Response object or error string
         */
        std::expected<Response, std::string> put(const std::string& path, const std::string& body);

        /**
         * @brief Send a DELETE request to the specified path
         * @param path The URL path to request
         * @return Expected Response object or error string
         */
        std::expected<Response, std::string> del(const std::string& path);

       protected:
        std::string host_;
        int port_;
        bool keep_alive_ = true;
        Logger& log = Logger::getInstance();

        /**
         * @brief Virtual method to send HTTP request
         * @param method The HTTP method (GET, POST, etc.)
         * @param path The URL path
         * @return Expected response string or error string
         */
        virtual std::expected<std::string, std::string> sendRequest(const std::string& method,
                                                                    const std::string& path) = 0;

        /**
         * @brief Create a new socket for connection
         * @return Socket file descriptor or -1 on failure
         */
        int createSocket() {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                log.debug("Failed to create socket {}", std::string(strerror(errno)));
            }
            return sock;
        }

        /**
         * @brief Close the given socket
         * @param sock The socket file descriptor to close
         */
        void closeSocket(int sock) {
            if (sock >= 0) {
                close(sock);
            }
        }

        /**
         * @brief Parse HTTP status code from raw response
         * @param raw_response The complete HTTP response string
         * @return Parsed Status enum value
         */
        http::Status parseStatus(const std::string& raw_response);

        /**
         * @brief Parse complete HTTP response from raw data
         * @param raw_response The complete HTTP response string
         * @return Expected Response object or error string
         */
        std::expected<Response, std::string> parseResponse(const std::string& raw_response);

        /**
         * @brief Parse chunked transfer encoding body
         * @param chunked_body The chunked body content
         * @return Expected parsed body string or error string
         */
        std::expected<std::string, std::string> parseChunkedBody(const std::string& chunked_body);
    };

    /**
     * @brief HTTP client implementation for plain HTTP connections
     *
     * This class handles HTTP requests over TCP sockets with support for
     * keep-alive connections and timeout management.
     */
    class HttpClient : public BaseClient {
       private:
        /**
         * @brief Send HTTP request over TCP socket
         * @param method The HTTP method to use
         * @param path The URL path to request
         * @return Expected response string or error string
         */
        std::expected<std::string, std::string> sendRequest(const std::string& method,
                                                            const std::string& path) override {
            int sock = createSocket();
            if (sock < 0) {
                return std::unexpected("Failed to create socket");
            }

            struct sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port_);

            struct hostent* server = gethostbyname(host_.c_str());
            if (!server) {
                closeSocket(sock);
                return std::unexpected("Host not found: " + host_);
            }

            memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

            struct timeval tv{};
            tv.tv_sec = CLIENT_TIMEOUT_SECONDS;
            tv.tv_usec = 0;

            if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0 ||
                setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
                closeSocket(sock);
                return std::unexpected("Failed to set socket timeout");
            }

            if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                closeSocket(sock);
                return std::unexpected("Connection failed to " + host_ + ":" + std::to_string(port_));
            }

            std::ostringstream oss;
            oss << method << " " << path << " HTTP/1.1\r\n";
            oss << "Host: " << host_ << "\r\n";
            oss << "Connection: " << (keep_alive_ ? "keep-alive" : "close") << "\r\n";
            oss << "\r\n";

            std::string request_str = oss.str();

            if (send(sock, request_str.data(), request_str.size(), 0) < 0) {
                closeSocket(sock);
                return std::unexpected("Failed to send request");
            }

            return readResponse(sock);
        }

        /**
         * @brief Read response from socket
         * @param sock The socket file descriptor
         * @return Expected response string or error string
         */
        std::expected<std::string, std::string> readResponse(int sock) {
            std::string response;
            std::array<char, READ_BUFFER_SIZE> buffer{};

            while (true) {
                int bytes_read = ::recv(sock, buffer.data(), buffer.size(), 0);

                if (bytes_read < 0) {
                    if (errno == EINTR) continue;
                    closeSocket(sock);
                    return std::unexpected("recv failed: " + std::to_string(errno));
                }

                if (bytes_read == 0) break;

                response.append(buffer.data(), static_cast<std::size_t>(bytes_read));

                // heuristic end-of-stream
                if (bytes_read < READ_BUFFER_SIZE) break;
            }

            closeSocket(sock);
            return response;
        }

       public:
        /**
         * @brief Construct HTTP client with host and port
         * @param host The target host name or IP address
         * @param port The target port number
         */
        HttpClient(const std::string& host, int port) {
            host_ = host;
            port_ = port;
        }
    };

#ifdef HTTP_OPENSSL_SUPPORT
    /**
     * @brief HTTPS client implementation for secure connections
     *
     * This class handles HTTPS requests using OpenSSL for TLS/SSL encryption.
     * It supports certificate validation and custom CA certificates.
     */
    class HttpsClient : public BaseClient {
       private:
        bool use_custom_ca_ = true;
        std::string ca_cert_file_;

        // SSL context for HTTPS
        SSL_CTX* ssl_ctx_;
        SSL* ssl_;

        /**
         * @brief Close socket and cleanup SSL resources
         * @param sock The socket file descriptor to close
         */
        void closeSocket(int sock) {
            if (ssl_) {
                SSL_free(ssl_);
                ssl_ = nullptr;
            }
            if (sock >= 0) {
                close(sock);
            }
        }

        /**
         * @brief Initialize OpenSSL context for secure connections
         * @return True on success, false on failure
         */
        bool initializeSSL() {
            SSL_library_init();
            SSL_load_error_strings();
            OpenSSL_add_ssl_algorithms();

            // Create SSL context
            const SSL_METHOD* method = TLS_client_method();
            ssl_ctx_ = SSL_CTX_new(method);
            if (!ssl_ctx_) {
                return false;
            }

            // Set minimum protocol version
            SSL_CTX_set_min_proto_version(ssl_ctx_, TLS1_2_VERSION);

            // Disable SSLv2 and SSLv3
            SSL_CTX_set_options(ssl_ctx_, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

            // Set default verify paths for certificate validation
            if (SSL_CTX_set_default_verify_paths(ssl_ctx_) != 1) {
                // Not critical, but might affect certificate validation
            }

            // If a custom CA certificate is provided, load it
            if (use_custom_ca_ && !ca_cert_file_.empty()) {
                if (SSL_CTX_load_verify_locations(ssl_ctx_, ca_cert_file_.c_str(), nullptr) != 1) {
                    SSL_CTX_free(ssl_ctx_);
                    ssl_ctx_ = nullptr;
                    return false;
                }

                // Set verification mode to verify peer
                SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_PEER, nullptr);
            }

            return true;
        }

        /**
         * @brief Cleanup all OpenSSL resources
         */
        void cleanupSSL() {
            if (ssl_) {
                SSL_shutdown(ssl_);
                SSL_free(ssl_);
                ssl_ = nullptr;
            }

            if (ssl_ctx_) {
                SSL_CTX_free(ssl_ctx_);
                ssl_ctx_ = nullptr;
            }

            EVP_cleanup();
        }

        /**
         * @brief Send HTTPS request over secure connection
         * @param method The HTTP method to use
         * @param path The URL path to request
         * @return Expected response string or error string
         */
        std::expected<std::string, std::string> sendRequest(const std::string& method,
                                                            const std::string& path) override {
            int sock = createSocket();
            if (sock < 0) {
                return std::unexpected("Failed to create socket");
            }

            struct sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port_);

            struct hostent* server = gethostbyname(host_.c_str());
            if (!server) {
                closeSocket(sock);
                return std::unexpected("Host not found: " + host_);
            }

            memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

            struct timeval tv{};
            tv.tv_sec = CLIENT_TIMEOUT_SECONDS;
            tv.tv_usec = 0;

            if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0 ||
                setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
                closeSocket(sock);
                return std::unexpected("Failed to set socket timeout");
            }

            if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                closeSocket(sock);
                return std::unexpected("Connection failed to " + host_ + ":" + std::to_string(port_));
            }

            // Initialize SSL context if needed
            if (!ssl_ctx_) {
                if (!initializeSSL()) {
                    closeSocket(sock);
                    return std::unexpected("Failed to initialize SSL context");
                }
            }

            ssl_ = SSL_new(ssl_ctx_);
            if (!ssl_) {
                closeSocket(sock);
                return std::unexpected("Failed to create SSL connection");
            }

            SSL_set_fd(ssl_, sock);

            int ret = SSL_connect(ssl_);
            if (ret <= 0) {
                int err = SSL_get_error(ssl_, ret);
                ERR_print_errors_fp(stderr);
                SSL_free(ssl_);
                closeSocket(sock);
                return std::unexpected("SSL handshake failed: " + std::to_string(err));
            }

            std::ostringstream oss;
            oss << method << " " << path << " HTTP/1.1\r\n";
            oss << "Host: " << host_ << "\r\n";
            oss << "Connection: " << (keep_alive_ ? "keep-alive" : "close") << "\r\n";
            oss << "\r\n";

            std::string request_str = oss.str();

            if (SSL_write(ssl_, request_str.data(), request_str.size()) <= 0) {
                SSL_free(ssl_);
                close(sock);
                return std::unexpected("Failed to send HTTPS request");
            }

            return readResponse(sock);
        }

        /**
         * @brief Read response from secure socket
         * @param sock The socket file descriptor
         * @return Expected response string or error string
         */
        std::expected<std::string, std::string> readResponse(int sock) {
            std::string response;
            std::array<char, READ_BUFFER_SIZE> buffer{};

            while (true) {
                int bytes_read = SSL_read(ssl_, buffer.data(), static_cast<int>(buffer.size()));

                if (bytes_read <= 0) {
                    int err = SSL_get_error(ssl_, bytes_read);

                    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                        continue;
                    }

                    return std::unexpected("SSL read failed with error: " + std::to_string(err));
                }

                response.append(buffer.data(), static_cast<std::size_t>(bytes_read));

                // heuristic end-of-stream
                if (bytes_read < READ_BUFFER_SIZE) break;
            }

            closeSocket(sock);
            return response;
        }

       public:
        /**
         * @brief Construct HTTPS client with host and port
         * @param host The target host name or IP address
         * @param port The target port number
         */
        HttpsClient(const std::string& host, int port) {
            host_ = host;
            port_ = port;
            ssl_ctx_ = nullptr;
            ssl_ = nullptr;
        }

        /**
         * @brief Destructor to cleanup SSL resources
         */
        ~HttpsClient() { cleanupSSL(); }

        /**
         * @brief Set custom CA certificate file for validation
         * @param cert_file Path to the CA certificate file
         */
        void setCert(const std::string& cert_file) {
            ca_cert_file_ = cert_file;
            use_custom_ca_ = true;

            // Check if SSL context exists
            if (ssl_ctx_ == nullptr) {
                // If no SSL context exists yet, we can't load the certificate
                // The SSL context will be initialized on first request
                return;
            }

            // If SSL context already exists, load the custom CA certificate
            if (SSL_CTX_load_verify_locations(ssl_ctx_, ca_cert_file_.c_str(), nullptr) != 1) {
                // Get detailed error information
                ERR_print_errors_fp(stderr);

                // Get the specific OpenSSL error
                unsigned long err = ERR_get_error();
                char err_buf[256];
                ERR_error_string_n(err, err_buf, sizeof(err_buf));

                log.debug("Failed to load CA certificate from '{}: {}", cert_file, std::string(err_buf));
            }

            // Set verification mode to verify peer
            SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_PEER, nullptr);
        }
    };

#endif

    /**
     * @brief Factory class for creating HTTP/HTTPS clients
     *
     * This factory parses URLs and creates appropriate client instances
     * based on the protocol (HTTP or HTTPS) specified in the URL.
     */
    class ClientFactory {
       public:
        /**
         * @brief Create client instance based on URL protocol
         * @param url The full URL to parse and create client for
         * @return Expected unique pointer to BaseClient or error string
         */
        static std::expected<std::unique_ptr<BaseClient>, std::string> createClient(const std::string& url) {
            // Parse URL
            std::string protocol;
            std::string host;
            std::string port_str;
            std::string path;

            // Find protocol (http:// or https://)
            size_t protocol_end = url.find("://");
            if (protocol_end == std::string::npos) {
                protocol = "http";
            } else {
                protocol = url.substr(0, protocol_end);
            }

            Protocol client_protocol = (protocol == "https") ? Protocol::HTTPS : Protocol::HTTP;

            // Get the part after ://
            std::string remaining = url.substr(protocol_end + 3);

            // Find host (everything up to first : or /)
            size_t host_end = remaining.find_first_of(":/");
            if (host_end == std::string::npos) {
                host = remaining;
            } else {
                host = remaining.substr(0, host_end);

                // Check if there's a port
                if (remaining[host_end] == ':') {
                    size_t port_start = host_end + 1;
                    size_t port_end = remaining.find('/', port_start);
                    if (port_end == std::string::npos) {
                        port_str = remaining.substr(port_start);
                    } else {
                        port_str = remaining.substr(port_start, port_end - port_start);
                    }
                }
            }

            // Parse port
            int port = 80;
            if (port_str.empty()) {
                port = (client_protocol == Protocol::HTTPS) ? 443 : 80;
            } else {
                auto parsed_port = parsePort(port_str);
                if (parsed_port.has_value()) {
                    port = parsed_port.value();
                }
            }

#ifdef HTTP_OPENSSL_SUPPORT
            if (client_protocol == Protocol::HTTPS) {
                return std::make_unique<HttpsClient>(host, port);
            } else {
                return std::make_unique<HttpClient>(host, port);
            }
#else
            return std::make_unique<HttpClient>(host, port);
#endif
        }

       private:
        /**
         * @brief Parse port number from string
         * @param port_str The port string to parse
         * @return Optional parsed port number or nullopt on error
         */
        static std::optional<int> parsePort(std::string_view port_str) {
            int value;

            auto res = std::from_chars(port_str.data(), port_str.data() + port_str.size(), value);

            if (res.ec != std::errc{} || res.ptr != port_str.data() + port_str.size()) {
                return std::nullopt;
            }

            return value;
        }
    };

    /**
     * @brief High-level client wrapper that uses the factory to create appropriate clients
     *
     * This class provides a simplified interface for making HTTP requests by
     * automatically creating the correct client type based on the URL.
     */
    class Client {
       private:
        std::unique_ptr<BaseClient> client_;
        Logger& log = Logger::getInstance();

       public:
        /**
         * @brief Construct client from URL
         * @param url The target URL to connect to
         */
        Client(const std::string& url) {
            auto result = ClientFactory::createClient(url);
            if (!result) {
                log.info("Failed to create client: {}", result.error());
            }
            client_ = std::move(result.value());
        }

        /**
         * @brief Send GET request
         * @param path The URL path to request
         * @return Expected Response object or error string
         */
        std::expected<Response, std::string> get(const std::string& path) { return client_->get(path); }

        /**
         * @brief Send POST request with body data
         * @param path The URL path to request
         * @param body The request body content
         * @return Expected Response object or error string
         */
        std::expected<Response, std::string> post(const std::string& path, const std::string& body) {
            return client_->post(path, body);
        }

        /**
         * @brief Send PUT request with body data
         * @param path The URL path to request
         * @param body The request body content
         * @return Expected Response object or error string
         */
        std::expected<Response, std::string> put(const std::string& path, const std::string& body) {
            return client_->put(path, body);
        }

        /**
         * @brief Send DELETE request
         * @param path The URL path to request
         * @return Expected Response object or error string
         */
        std::expected<Response, std::string> del(const std::string& path) { return client_->del(path); }
    };

}  // namespace http

#endif