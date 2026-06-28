
#include "client.h"

#include "utils.h"

namespace http {

    std::expected<Response, std::string> BaseClient::get(const std::string& path) {
        auto response = sendRequest("GET", path);
        if (!response) {
            return std::unexpected(response.error());
        }

        return parseResponse(*response);
    }

    std::expected<Response, std::string> BaseClient::post(const std::string& path, const std::string& body) {
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

    std::expected<Response, std::string> BaseClient::put(const std::string& path, const std::string& body) {
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

    std::expected<Response, std::string> BaseClient::del(const std::string& path) {
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

    std::expected<Response, std::string> BaseClient::parseResponse(const std::string& raw_response) {
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

            std::string key = utils::trim(line.substr(0, colon_pos));
            std::string value = utils::trim(line.substr(colon_pos + 1));

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

    std::expected<std::string, std::string> BaseClient::parseChunkedBody(const std::string& chunked_body) {
        std::string result;
        size_t pos = 0;

        while (pos < chunked_body.size()) {
            size_t line_end = chunked_body.find("\r\n", pos);
            if (line_end == std::string::npos) {
                return std::unexpected("Invalid chunked encoding: missing size line");
            }

            std::string size_str = chunked_body.substr(pos, line_end - pos);

            size_t chunk_size = 0;
            if (!utils::parseHexSize(size_str, chunk_size)) {
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

    http::Status BaseClient::parseStatus(const std::string& raw_response) {
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