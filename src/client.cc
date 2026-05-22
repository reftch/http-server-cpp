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

    std::optional<Response> Client::Post(const std::string& path, const std::string& body) {
        try {
            std::ostringstream request;
            request << "POST " << path << " HTTP/1.1\r\n";
            request << "Host: " << host << "\r\n";
            request << "Content-Type: application/json\r\n";
            request << "Content-Length: " << body.length() << "\r\n";
            request << "Connection: " << (keep_alive_ ? "keep-alive" : "close") << "\r\n";
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
            request << "Connection: " << (keep_alive_ ? "keep-alive" : "close") << "\r\n";
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
        if (sock < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        struct sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        struct hostent* server = gethostbyname(host.c_str());
        if (!server) {
            close(sock);
            throw std::runtime_error("Host not found: " + host);
        }

        memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

        // Set timeout
        struct timeval tv{};
        tv.tv_sec = timeout_seconds;
        tv.tv_usec = 0;

        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0 ||
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
            close(sock);
            throw std::runtime_error("Failed to set socket timeout");
        }

        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sock);
            throw std::runtime_error("Connection failed to " + host + ":" + std::to_string(port));
        }

        // Prepare request
        std::ostringstream oss;
        oss << method << " " << path << " HTTP/1.1\r\n";
        oss << "Host: " << host << "\r\n";
        oss << "Connection: " << (keep_alive_ ? "keep-alive" : "close") << "\r\n";
        oss << "\r\n";

        const std::string& request_str = oss.str();

        if (send(sock, request_str.data(), request_str.size(), 0) < 0) {
            close(sock);
            throw std::runtime_error("Failed to send request");
        }

        // Read response with minimal overhead
        std::string response;
        char buffer[4096];
        ssize_t bytes_read;

        // Make socket non-blocking
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        struct pollfd pfd{};
        pfd.fd = sock;
        pfd.events = POLLIN;

        int timeout_ms = timeout_seconds * 1000;
        int elapsed_ms = 0;
        int poll_interval = 1;

        while (elapsed_ms < timeout_ms) {
            int poll_result = poll(&pfd, 1, poll_interval);
            if (poll_result < 0) {
                close(sock);
                throw std::runtime_error("Poll failed");
            } else if (poll_result == 0) {
                break;
            }

            if (pfd.revents & POLLIN) {
                bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);
                if (bytes_read < 0) {
                    close(sock);
                    throw std::runtime_error("Receive failed");
                } else if (bytes_read == 0) {
                    break;
                } else {
                    buffer[bytes_read] = '\0';
                    response += buffer;
                    elapsed_ms = 0;
                }
            } else {
                break;
            }
        }

        close(sock);
        return response;
    }

    Response Client::parse_response(const std::string& raw_response) {
        Response response;

        size_t status_start = raw_response.find("HTTP/1.1 ");
        if (status_start == std::string::npos) {
            status_start = raw_response.find("HTTP/1.0 ");
        }

        Status status = parse_status(raw_response);

        size_t header_end = raw_response.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            std::string body = raw_response.substr(header_end + 4);
            response.SetContentByType(body, "text/plain", status);
        } else {
            response.SetContentByType(raw_response, "text/plain", status);
        }

        return response;
    }

    Status Client::parse_status(const std::string& raw_response) {
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

}  // namespace http