#include "client.h"

namespace http {

    Client::Client(const std::string& url) {
        // Parse URL
        std::regex url_regex("^(https?)://([^:/]+)(:(\\d+))?(/.*)?$");
        std::smatch match;

        if (std::regex_search(url, match, url_regex)) {
            is_https = (match[1].str() == "https");
            host = match[2].str();
            port = match[4].str().empty() ? (is_https ? 443 : 80) : std::stoi(match[4].str());
        } else {
            throw std::runtime_error("Invalid URL format: " + url);
        }
    }

    std::optional<Response> Client::Get(const std::string& path) {
        try {
            std::string response = send_request("GET", path);
            return parse_response(response);
        } catch (const std::exception& e) {
            std::cerr << "Error in GET request: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    std::optional<Response> Client::Post(const std::string& path, const std::string& body = "") {
        try {
            std::ostringstream request;
            request << "POST " << path << " HTTP/1.1\r\n";
            request << "Host: " << host << "\r\n";
            request << "Content-Type: application/json\r\n";
            request << "Content-Length: " << body.length() << "\r\n";
            request << "Connection: close\r\n";
            request << "\r\n";
            request << body;

            std::string response = send_request("POST", path);
            return parse_response(response);
        } catch (const std::exception& e) {
            std::cerr << "Error in POST request: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    std::optional<Response> Client::Put(const std::string& path, const std::string& body) {
        try {
            std::ostringstream request;
            request << "PUT " << path << " HTTP/1.1\r\n";
            request << "Host: " << host << "\r\n";
            request << "Content-Type: application/json\r\n";
            request << "Content-Length: " << body.length() << "\r\n";
            request << "Connection: close\r\n";
            request << "\r\n";
            request << body;

            std::string response = send_request("PUT", path);
            return parse_response(response);
        } catch (const std::exception& e) {
            std::cerr << "Error in PUT request: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    std::optional<Response> Client::Delete(const std::string& path) {
        try {
            std::string response = send_request("DELETE", path);
            return parse_response(response);
        } catch (const std::exception& e) {
            std::cerr << "Error in DELETE request: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    int Client::create_socket() {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
        }
        return sock;
    }

    std::string Client::send_request(const std::string& method, const std::string& path) {
        int sock = create_socket();

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        // Convert host to IP address
        struct hostent* server = gethostbyname(host.c_str());
        if (!server) {
            close(sock);
            throw std::runtime_error("Host not found: " + host);
        }

        memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

        // Set timeout for connection
        struct timeval tv;
        tv.tv_sec = timeout_seconds;
        tv.tv_usec = 0;

        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            close(sock);
            throw std::runtime_error("Failed to set receive timeout: " + std::string(strerror(errno)));
        }

        if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
            close(sock);
            throw std::runtime_error("Failed to set send timeout: " + std::string(strerror(errno)));
        }

        // Try to connect
        int connect_result = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (connect_result < 0) {
            close(sock);
            throw std::runtime_error("Connection failed to " + host + ":" + std::to_string(port) + " - " +
                                     std::string(strerror(errno)));
        }

        // Create HTTP request
        std::ostringstream request;
        request << method << " " << path << " HTTP/1.1\r\n";
        request << "Host: " << host << "\r\n";
        request << "Connection: close\r\n";
        request << "\r\n";

        std::string request_str = request.str();
        ssize_t sent = send(sock, request_str.c_str(), request_str.length(), 0);
        if (sent < 0) {
            close(sock);
            throw std::runtime_error("Failed to send request: " + std::string(strerror(errno)));
        }

        // Read response using poll - immediately get the response
        std::string response;
        char buffer[4096];
        ssize_t bytes_read;

        // Set socket to non-blocking mode for immediate read
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        // Poll for data with very short timeout
        struct pollfd pfd;
        pfd.fd = sock;
        pfd.events = POLLIN;

        // Try to read immediately, then poll for more data
        bool has_data = false;
        while (true) {
            int poll_result = poll(&pfd, 1, 100);  // 100ms timeout

            if (poll_result < 0) {
                close(sock);
                throw std::runtime_error("Poll failed: " + std::string(strerror(errno)));
            } else if (poll_result == 0) {
                // No data available, but we should still try to read what we can
                // This gives us immediate response if data is already there
                break;
            }

            if (pfd.revents & POLLIN) {
                bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);
                if (bytes_read <= 0) {
                    break;
                }
                buffer[bytes_read] = '\0';
                response += buffer;
                has_data = true;
            }
        }

        // If we got data, we're done
        if (has_data) {
            close(sock);
            return response;
        }

        // If no immediate data, read with proper timeout
        while (true) {
            int poll_result = poll(&pfd, 1, timeout_seconds * 1000);
            if (poll_result < 0) {
                close(sock);
                throw std::runtime_error("Poll failed: " + std::string(strerror(errno)));
            } else if (poll_result == 0) {
                close(sock);
                throw std::runtime_error("Timeout waiting for response");
            }

            if (pfd.revents & POLLIN) {
                bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);
                if (bytes_read <= 0) {
                    break;
                }
                buffer[bytes_read] = '\0';
                response += buffer;
            }
        }

        close(sock);
        return response;
    }

    http::Response Client::parse_response(const std::string& raw_response) {
        http::Response response;

        // Find status code manually without regex
        size_t status_start = raw_response.find("HTTP/1.1 ");
        if (status_start == std::string::npos) {
            status_start = raw_response.find("HTTP/1.0 ");
        }

        // Parse status
        http::Status status = parse_status(raw_response);

        // Extract body (everything after headers)
        size_t header_end = raw_response.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            std::string body = raw_response.substr(header_end + 4);
            // Set the body content
            response.SetContentByType(body, "text/plain", status);
        } else {
            // If no headers found, the whole response is the body
            response.SetContentByType(raw_response, "text/plain", status);
        }

        return response;
    }

    http::Status Client::parse_status(const std::string& raw_response) {
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
                        return http::Status::ok;
                    case 201:
                        return http::Status::created;
                    case 202:
                        return http::Status::accepted;
                    case 204:
                        return http::Status::no_content;
                    case 300:
                        return http::Status::multiple_choices;
                    case 301:
                        return http::Status::moved_permanently;
                    case 302:
                        return http::Status::moved_temporarily;
                    case 304:
                        return http::Status::not_modified;
                    case 400:
                        return http::Status::bad_request;
                    case 401:
                        return http::Status::unauthorized;
                    case 403:
                        return http::Status::forbidden;
                    case 404:
                        return http::Status::not_found;
                    case 500:
                        return http::Status::internal_server_error;
                    case 501:
                        return http::Status::not_implemented;
                    case 502:
                        return http::Status::bad_gateway;
                    case 503:
                        return http::Status::service_unavailable;
                    default:
                        return http::Status::ok;
                }
            }
        }

        return http::Status::ok;
    }

}  // namespace http