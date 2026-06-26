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
#include <iostream>
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

    class Client {
       public:
        Client(const std::string& url) {
            // Parse URL
            std::string protocol;
            std::string host;
            std::string port_str;
            std::string path;

            // Find protocol (http:// or https://)
            size_t protocol_end = url.find("://");
            if (protocol_end == std::string::npos) {
                protocol = "http";
            }

            protocol = url.substr(0, protocol_end);
            is_https_ = (protocol == "https");

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

            host_ = host;

            // Parse port
            if (port_str.empty()) {
                port_ = is_https_ ? 443 : 80;
            } else {
                auto port = parsePort(port_str);
                if (port.has_value()) {
                    port_ = port.value();
                }
            }

// Initialize SSL context for HTTPS
#ifdef HTTP_OPENSSL_SUPPORT
            if (is_https_) {
                if (!initializeSSL()) {
                    log.debug("Failed to initialize SSL context");
                }
            }
#endif
        }

        std::expected<Response, std::string> get(const std::string& path) {
            auto response = sendRequest("GET", path);
            if (!response) {
                return std::unexpected(response.error());
            }

            return parseResponse(*response);
        }

        std::expected<Response, std::string> post(const std::string& path, const std::string& body) {
            std::ostringstream request;
            request << "POST " << path << " HTTP/1.1\r\n";
            request << "Host: " << host_ << "\r\n";
            request << "Content-Type: application/json\r\n";
            request << "Content-Length: " << body.length() << "\r\n";
            request << "Connection: " << (keep_alive_ ? "keep-alive" : "close") << "\r\n";
            request << "\r\n";
            request << body;

            auto response = sendRequest("POST", path);
            if (!response) {
                return std::unexpected(response.error());
            }

            auto parsed = parseResponse(*response);
            if (!parsed) {
                return std::unexpected(parsed.error());
            }

            return *parsed;
        }

        std::expected<Response, std::string> put(const std::string& path, const std::string& body) {
            std::ostringstream request;
            request << "PUT " << path << " HTTP/1.1\r\n";
            request << "Host: " << host_ << "\r\n";
            request << "Content-Type: application/json\r\n";
            request << "Content-Length: " << body.length() << "\r\n";
            request << "Connection: " << (keep_alive_ ? "keep-alive" : "close") << "\r\n";
            request << "\r\n";
            request << body;

            auto response = sendRequest("PUT", path);
            if (!response) {
                return std::unexpected(response.error());
            }

            auto parsed = parseResponse(*response);
            if (!parsed) {
                return std::unexpected(parsed.error());
            }

            return *parsed;
        }

        std::expected<Response, std::string> del(const std::string& path) {
            auto response = sendRequest("DELETE", path);

            if (!response) {
                return std::unexpected(response.error());
            }

            auto parsed = parseResponse(*response);
            if (!parsed) {
                return std::unexpected(parsed.error());
            }

            return *parsed;
        }

       private:
        std::string host_;
        int port_;
        bool is_https_ = false;
        bool keep_alive_ = true;
        // certificate
        std::string ca_cert_file_;

        const int kTIMEOUT_SECONDS = CLIENT_TIMEOUT_SECONDS;

        // logger
        Logger& log = Logger::getInstance();
#ifdef HTTP_OPENSSL_SUPPORT
        bool use_custom_ca_ = true;
        // SSL context for HTTPS
        SSL_CTX* ssl_ctx_;
        SSL* ssl_;
#endif

        int createSocket() {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                log.debug("Failed to create socket {}", std::string(strerror(errno)));
            }
            return sock;
        }

        void closeSocket(int sock) {
#ifdef HTTP_OPENSSL_SUPPORT
            if (ssl_) {
                SSL_free(ssl_);
                ssl_ = nullptr;
            }
#endif
            if (sock >= 0) {
                close(sock);
            }
        }

        std::expected<std::string, std::string> sendRequest(const std::string& method, const std::string& path) {
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
            tv.tv_sec = kTIMEOUT_SECONDS;
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

#ifdef HTTP_OPENSSL_SUPPORT
            if (is_https_) {
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
            }
#endif

            std::ostringstream oss;
            oss << method << " " << path << " HTTP/1.1\r\n";
            oss << "Host: " << host_ << "\r\n";
            oss << "Connection: " << (keep_alive_ ? "keep-alive" : "close") << "\r\n";
            oss << "\r\n";

            std::string request_str = oss.str();

            if (is_https_) {
#ifdef HTTP_OPENSSL_SUPPORT
                if (SSL_write(ssl_, request_str.data(), request_str.size()) <= 0) {
                    SSL_free(ssl_);
                    close(sock);
                    return std::unexpected("Failed to send HTTPS request");
                }
#endif
            } else {
                if (send(sock, request_str.data(), request_str.size(), 0) < 0) {
                    closeSocket(sock);
                    return std::unexpected("Failed to send request");
                }
            }

            return readResponse(sock);
        }

        std::expected<std::string, std::string> readResponse(int sock) {
            std::string response;
            std::array<char, READ_BUFFER_SIZE> buffer{};

            while (true) {
                int bytes_read = 0;

                if (is_https_) {
#ifdef HTTP_OPENSSL_SUPPORT
                    bytes_read = SSL_read(ssl_, buffer.data(), static_cast<int>(buffer.size()));

                    if (bytes_read <= 0) {
                        int err = SSL_get_error(ssl_, bytes_read);

                        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                            continue;
                        }

                        return std::unexpected("SSL read failed with error: " + std::to_string(err));
                    }
#endif
                } else {
                    bytes_read = ::recv(sock, buffer.data(), buffer.size(), 0);

                    if (bytes_read < 0) {
                        if (errno == EINTR) continue;

                        return std::unexpected("recv failed: " + std::to_string(errno));
                    }

                    if (bytes_read == 0) break;
                }

                response.append(buffer.data(), static_cast<std::size_t>(bytes_read));

                // heuristic end-of-stream
                if (bytes_read < READ_BUFFER_SIZE) break;
            }

            closeSocket(sock);

            return response;
        }

        std::expected<Response, std::string> parseResponse(const std::string& raw_response) {
            Response response;

            size_t header_end = raw_response.find("\r\n\r\n");
            if (header_end == std::string::npos) {
                return std::unexpected("Invalid HTTP response: missing header terminator");
            }

            Status status = parseStatus(raw_response);

            std::string header_section = raw_response.substr(0, header_end);
            std::istringstream iss(header_section);
            std::string line;

            while (std::getline(iss, line)) {
                if (line.empty()) break;

                size_t colon_pos = line.find(':');
                if (colon_pos == std::string::npos) continue;

                std::string key = trim(line.substr(0, colon_pos));
                std::string value = trim(line.substr(colon_pos + 1));

                response.setHeader(key, value);
            }

            std::string body = raw_response.substr(header_end + 4);

            const auto& headers = response.headers();
            auto it = headers.find("Transfer-Encoding");
            if (it != response.headers().end() && it->second == "chunked") {
                auto chunked = parseChunkedBody(body);
                if (!chunked) {
                    return std::unexpected(chunked.error());
                }

                body = *chunked;
            }

            response << status << body;

            return response;
        }

        std::expected<std::string, std::string> parseChunkedBody(const std::string& chunked_body) {
            std::string result;
            size_t pos = 0;

            while (pos < chunked_body.size()) {
                size_t line_end = chunked_body.find("\r\n", pos);
                if (line_end == std::string::npos) {
                    return std::unexpected("Invalid chunked encoding: missing size line");
                }

                std::string size_str = chunked_body.substr(pos, line_end - pos);

                size_t chunk_size = 0;
                if (!parseHexSize(size_str, chunk_size)) {
                    return std::unexpected("Invalid chunk size");
                }

                pos = line_end + 2;

                if (chunk_size == 0) {
                    break;
                }

                if (pos + chunk_size > chunked_body.size()) {
                    return std::unexpected("Truncated chunk body");
                }

                result.append(chunked_body, pos, chunk_size);
                pos += chunk_size + 2;  // skip \r\n
            }

            return result;
        }

        http::Status parseStatus(const std::string& raw_response) {
            size_t status_start = raw_response.find("HTTP/1.1 ");
            if (status_start == std::string::npos) {
                status_start = raw_response.find("HTTP/1.0 ");
            }

            if (status_start != std::string::npos) {
                size_t status_end = raw_response.find(' ', status_start + 9);
                if (status_end != std::string::npos) {
                    std::string status_str = raw_response.substr(status_start + 9, status_end - (status_start + 9));
                    int status_code = std::stoi(status_str);

                    switch (status_code) {
                        case 200:
                            return Status::ok;
                        case 201:
                            return Status::created;
                        case 202:
                            return Status::accepted;
                        case 204:
                            return Status::no_content;
                        case 300:
                            return Status::multiple_choices;
                        case 301:
                            return Status::moved_permanently;
                        case 302:
                            return Status::moved_temporarily;
                        case 304:
                            return Status::not_modified;
                        case 400:
                            return Status::bad_request;
                        case 401:
                            return Status::unauthorized;
                        case 403:
                            return Status::forbidden;
                        case 404:
                            return Status::not_found;
                        case 500:
                            return Status::internal_server_error;
                        case 501:
                            return Status::not_implemented;
                        case 502:
                            return Status::bad_gateway;
                        case 503:
                            return Status::service_unavailable;
                        default:
                            return Status::ok;
                    }
                }
            }

            return Status::ok;
        }

        static std::optional<int> parsePort(std::string_view port_str) {
            int value;

            auto res = std::from_chars(port_str.data(), port_str.data() + port_str.size(), value);

            if (res.ec != std::errc{} || res.ptr != port_str.data() + port_str.size()) {
                return std::nullopt;
            }

            return value;
        }

        static std::string trim(const std::string& str) {
            size_t start = str.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) {
                return "";
            }

            size_t end = str.find_last_not_of(" \t\r\n");
            return str.substr(start, end - start + 1);
        }

        static bool parseHexSize(const std::string& s, size_t& out) {
            out = 0;

            for (char c : s) {
                out <<= 4;

                if (c >= '0' && c <= '9')
                    out += c - '0';
                else if (c >= 'a' && c <= 'f')
                    out += c - 'a' + 10;
                else if (c >= 'A' && c <= 'F')
                    out += c - 'A' + 10;
                else
                    return false;
            }

            return true;
        }

#ifdef HTTP_OPENSSL_SUPPORT
        // SSL helper functions
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
            // if (use_custom_ca_ && !ca_cert_file_.empty()) {
            //     if (SSL_CTX_load_verify_locations(ssl_ctx_, ca_cert_file_.c_str(), nullptr) != 1) {
            //         SSL_CTX_free(ssl_ctx_);
            //         ssl_ctx_ = nullptr;
            //         return false;
            //     }

            //     // Set verification mode to verify peer
            //     SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_PEER, nullptr);
            // }

            return true;
        }

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

#endif
    };
}  // namespace http

#endif
