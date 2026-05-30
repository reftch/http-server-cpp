#include "client.h"

namespace http {

    Client::Client(const std::string& url) {
        // Parse URL
        std::string protocol;
        std::string host;
        std::string port_str;
        std::string path;

        // Find protocol (http:// or https://)
        size_t protocol_end = url.find("://");
        if (protocol_end == std::string::npos) {
            throw std::runtime_error("Invalid URL format: " + url);
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
            try {
                port_ = std::stoi(port_str);
            } catch (const std::exception&) {
                throw std::runtime_error("Invalid port in URL: " + url);
            }
        }

        // Initialize SSL context for HTTPS
        if (is_https_) {
            if (!InitializeSSL()) {
                throw std::runtime_error("Failed to initialize SSL context");
            }
        }
    }

    std::optional<Response> Client::Get(const std::string& path) {
        try {
            std::string response = SendRequest("GET", path);
            return ParseResponse(response);
        } catch (const std::exception& e) {
            std::cerr << "Error in GET request: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    std::optional<Response> Client::Post(const std::string& path, const std::string& body) {
        try {
            std::ostringstream request;
            request << "POST " << path << " HTTP/1.1\r\n";
            request << "Host: " << host_ << "\r\n";
            request << "Content-Type: application/json\r\n";
            request << "Content-Length: " << body.length() << "\r\n";
            request << "Connection: " << (keep_alive_ ? "keep-alive" : "close") << "\r\n";
            request << "\r\n";
            request << body;

            std::string response = SendRequest("POST", path);
            return ParseResponse(response);
        } catch (const std::exception& e) {
            std::cerr << "Error in POST request: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    std::optional<Response> Client::Put(const std::string& path, const std::string& body) {
        try {
            std::ostringstream request;
            request << "PUT " << path << " HTTP/1.1\r\n";
            request << "Host: " << host_ << "\r\n";
            request << "Content-Type: application/json\r\n";
            request << "Content-Length: " << body.length() << "\r\n";
            request << "Connection: " << (keep_alive_ ? "keep-alive" : "close") << "\r\n";
            request << "\r\n";
            request << body;

            std::string response = SendRequest("PUT", path);
            return ParseResponse(response);
        } catch (const std::exception& e) {
            std::cerr << "Error in PUT request: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    std::optional<Response> Client::Delete(const std::string& path) {
        try {
            std::string response = SendRequest("DELETE", path);
            return ParseResponse(response);
        } catch (const std::exception& e) {
            std::cerr << "Error in DELETE request: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    int Client::CreateSocket() {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
        }
        return sock;
    }

    std::string Client::SendRequest(const std::string& method, const std::string& path) {
        int sock = CreateSocket();
        if (sock < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        struct sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);

        struct hostent* server = gethostbyname(host_.c_str());
        if (!server) {
            close(sock);
            throw std::runtime_error("Host not found: " + host_);
        }

        memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

        // Set timeout
        struct timeval tv{};
        tv.tv_sec = kTIMEOUT_SECONDS;
        tv.tv_usec = 0;

        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0 ||
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
            close(sock);
            throw std::runtime_error("Failed to set socket timeout");
        }

        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sock);
            throw std::runtime_error("Connection failed to " + host_ + ":" + std::to_string(port_));
        }

        if (is_https_) {
            // Create SSL connection
            ssl_ = SSL_new(ssl_ctx_);
            if (!ssl_) {
                close(sock);
                throw std::runtime_error("Failed to create SSL connection");
            }

            SSL_set_fd(ssl_, sock);

            // Perform SSL handshake
            int ret = SSL_connect(ssl_);
            if (ret <= 0) {
                int err = SSL_get_error(ssl_, ret);
                ERR_print_errors_fp(stderr);
                SSL_free(ssl_);
                close(sock);
                throw std::runtime_error("SSL handshake failed with error code: " + std::to_string(err));
            }
        }

        // Prepare request
        std::ostringstream oss;
        oss << method << " " << path << " HTTP/1.1\r\n";
        oss << "Host: " << host_ << "\r\n";
        oss << "Connection: " << (keep_alive_ ? "keep-alive" : "close") << "\r\n";
        oss << "\r\n";

        const std::string& request_str = oss.str();

        if (is_https_) {
            if (SSL_write(ssl_, request_str.data(), request_str.size()) <= 0) {
                SSL_free(ssl_);
                close(sock);
                throw std::runtime_error("Failed to send request");
            }
        } else {
            if (send(sock, request_str.data(), request_str.size(), 0) < 0) {
                close(sock);
                throw std::runtime_error("Failed to send request");
            }
        }

        // Read response
        return ReadResponse(sock);
    }

    static std::string Trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return "";
        }

        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    std::string Client::ReadResponse(int sock) {
        std::string response;
        std::array<char, READ_BUFFER_SIZE> buffer{};

        while (true) {
            int bytes_read = 0;
            if (ssl_) {
                bytes_read = SSL_read(ssl_, buffer.data(), static_cast<int>(buffer.size()));
                if (bytes_read <= 0) {
                    int err = SSL_get_error(ssl_, bytes_read);
                    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) continue;
                    break;
                }
            } else {
                bytes_read = ::recv(sock, buffer.data(), buffer.size(), 0);
                if (bytes_read < 0) {
                    if (errno == EINTR) continue;
                    break;
                }
                if (bytes_read == 0) break;
            }

            response.append(buffer.data(), static_cast<std::size_t>(bytes_read));
            // have all the bytes been read yet?
            if (bytes_read < READ_BUFFER_SIZE) break;
        }

        return Trim(response);
    }

    // Add this parsing method
    // Response Client::ParseResponse(const std::string& raw_response) {
    //     // static Response Parse(const std::string& response_string) {
    //     Response response;

    //     // Split the response into lines
    //     std::vector<std::string> lines;
    //     std::istringstream iss(raw_response);
    //     std::string line;

    //     while (std::getline(iss, line)) {
    //         lines.push_back(line);
    //     }

    //     if (lines.empty()) {
    //         return response;
    //     }

    //     Status status = ParseStatus(raw_response);

    //     // Parse headers (skip the first line and any empty lines)
    //     for (size_t i = 1; i < lines.size(); ++i) {
    //         if (lines[i].empty()) {
    //             // Empty line indicates end of headers
    //             break;
    //         }

    //         size_t colon_pos = lines[i].find(':');
    //         if (colon_pos != std::string::npos) {
    //             std::string key = lines[i].substr(0, colon_pos);
    //             std::string value = lines[i].substr(colon_pos + 1);

    //             // Trim whitespace from key and value
    //             key = Trim(key);
    //             value = Trim(value);

    //             response.set_header(key, value);
    //         }
    //     }

    //     // get response body
    //     std::string body = "";
    //     size_t header_end = raw_response.find("\r\n\r\n");
    //     std::cout << raw_response << '\n';
    //     if (header_end != std::string::npos) {
    //         body = raw_response.substr(header_end + 8);
    //     }

    //     response.SetContentByType(body, status);
    //     return response;
    // }

    Response Client::ParseResponse(const std::string& raw_response) {
        Response response;

        // Find the end of headers (after \r\n\r\n)
        size_t header_end = raw_response.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            return response;
        }

        // Parse status line (first line)
        Status status = ParseStatus(raw_response);

        // Parse headers
        std::string header_section = raw_response.substr(0, header_end);
        std::istringstream iss(header_section);
        std::string line;

        while (std::getline(iss, line)) {
            if (line.empty()) break;  // End of headers

            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);

                // Trim whitespace
                key = Trim(key);
                value = Trim(value);

                response.set_header(key, value);
            }
        }

        // Extract body (after headers)
        std::string body;

        // Check if it's chunked encoding
        std::string transfer_encoding = response.headers()["Transfer-Encoding"];
        if (!transfer_encoding.empty()) {
            std::string transfer_encoding = response.headers().at("Transfer-Encoding");
            if (transfer_encoding == "chunked") {
                body = ParseChunkedBody(body);
            }
        } else {
            body = raw_response.substr(header_end + 4);  // +4 for \r\n\r\n
        }

        response.SetContentByType(body, status);
        return response;
    }

    // Helper function to parse chunked body
    std::string Client::ParseChunkedBody(const std::string& chunked_body) {
        std::string result;
        size_t pos = 0;

        while (pos < chunked_body.length()) {
            // Read chunk size (hexadecimal)
            size_t chunk_size_end = chunked_body.find('\r', pos);
            if (chunk_size_end == std::string::npos) break;

            std::string chunk_size_str = chunked_body.substr(pos, chunk_size_end - pos);
            size_t chunk_size = std::stoi(chunk_size_str, nullptr, 16);

            pos = chunk_size_end + 2;  // Skip \r\n

            if (chunk_size == 0) {
                break;  // End of chunks
            }

            // Extract chunk data
            if (pos + chunk_size <= chunked_body.length()) {
                result += chunked_body.substr(pos, chunk_size);
                pos += chunk_size + 2;  // Skip \r\n after chunk
            } else {
                break;
            }
        }

        return result;
    }

    Status Client::ParseStatus(const std::string& raw_response) {
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

    bool Client::InitializeSSL() {
        // Initialize OpenSSL
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

    void Client::CleanupSSL() {
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

    void Client::SetCert(const std::string& cert_file) {
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

            throw std::runtime_error("Failed to load CA certificate from '" + cert_file + "': " + std::string(err_buf));
        }

        // Set verification mode to verify peer
        SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_PEER, nullptr);
    }

}  // namespace http