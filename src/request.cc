#include "request.h"

#include <iostream>
// #include "router.h"
#include "utils.h"

namespace http {

    Request::Request(const std::string& raw_request) {
        size_t line_end = raw_request.find("\r\n");
        if (line_end == std::string::npos) return;

        parseRequestLine(std::string_view(raw_request).substr(0, line_end));

        size_t pos = line_end + 2;
        parseHeaders(std::string_view(raw_request), pos);

        // get request body
        size_t header_end = raw_request.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            body_ = raw_request.substr(header_end + 4);
        }
    }

    void Request::parseRequestLine(std::string_view line) {
        size_t m1 = line.find(' ');
        size_t m2 = line.find(' ', m1 + 1);
        if (m1 == std::string_view::npos || m2 == std::string_view::npos) return;

        method_ = std::string(line.substr(0, m1));
        path_ = std::string(line.substr(m1 + 1, m2 - m1 - 1));
        version_ = std::string(line.substr(m2 + 1));
        mime_type_ = getMimeType(path_);
        params_.clear();
        query_.clear();

        // parse query string
        std::string query_string;
        size_t query_pos = path_.find('?');
        if (query_pos != std::string::npos) {
            normalize_path_ = path_.substr(0, query_pos);
            query_string_ = path_.substr(query_pos + 1);
            parseQueryString(query_string_);
        } else {
            normalize_path_ = path_;
        }
    }

    void Request::parseHeaders(std::string_view raw_request, size_t& pos) {
        while (pos < raw_request.size()) {
            size_t end = raw_request.find("\r\n", pos);
            if (end == std::string_view::npos) break;
            if (end == pos) {
                pos = end + 2;
                break;
            }

            size_t colon = raw_request.find(':', pos);
            if (colon != std::string_view::npos && colon < end) {
                std::string_view name = raw_request.substr(pos, colon - pos);

                size_t value_start = colon + 1;
                if (value_start < end && raw_request[value_start] == ' ') {
                    ++value_start;
                }

                std::string_view value = raw_request.substr(value_start, end - value_start);
                headers_[name] = value;
            }

            pos = end + 2;
        }
    }

    void Request::parseQueryString(const std::string& query_string) {
        if (query_string.empty()) {
            return;
        }

        size_t start = 0;
        while (start < query_string.length()) {
            size_t ampersand_pos = query_string.find('&', start);
            if (ampersand_pos == std::string::npos) {
                ampersand_pos = query_string.length();
            }

            size_t equals_pos = query_string.find('=', start);
            std::string key, value;

            if (equals_pos != std::string::npos && equals_pos < ampersand_pos) {
                key = query_string.substr(start, equals_pos - start);
                value = query_string.substr(equals_pos + 1, ampersand_pos - equals_pos - 1);
            } else {
                key = query_string.substr(start, ampersand_pos - start);
                value = "";
            }

            // URL decode and store
            key = ::utils::urlDecode(key);
            value = ::utils::urlDecode(value);
            query_[key] = value;

            start = ampersand_pos + 1;
        }
    }

    bool Request::isKeepAlive() {
        // HTTP/1.1 default is keep-alive
        // Only close if explicitly requested
        auto it = headers_.find("Connection");
        if (it == headers_.end()) {
            // No Connection header - assume keep-alive (HTTP/1.1 default)
            return true;
        }

        // Check if explicitly closed
        return it->second != "close";
    }

    std::string Request::getMimeType(const std::string& path) {
        // Extract just the filename part (before any query parameters)
        std::string clean_path = path;
        size_t query_pos = path.find('?');
        if (query_pos != std::string::npos) {
            clean_path = path.substr(0, query_pos);
        }

        // Extract file extension
        std::string fileExtension = clean_path.substr(clean_path.find_last_of(".") + 1);

        // Convert to lowercase for case-insensitive matching
        std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);

        // If no extension found, return empty string
        if (fileExtension.empty()) {
            return "";
        }

        static const std::map<std::string, const char*> mimeTypes = {
            {"html", "text/html"},        {"htm", "text/html"},
            {"css", "text/css"},          {"js", "text/javascript"},
            {"jpg", "image/jpeg"},        {"jpeg", "image/jpeg"},
            {"png", "image/png"},         {"gif", "image/gif"},
            {"svg", "image/svg+xml"},     {"xml", "application/xml"},
            {"json", "application/json"}, {"txt", "text/plain"},
            {"pdf", "application/pdf"},   {"mp3", "audio/mpeg"},
            {"mp4", "video/mp4"},         {"webm", "video/webm"},
            {"woff", "font/woff"},        {"woff2", "font/woff2"},
            {"ttf", "font/ttf"},          {"eot", "application/vnd.ms-fontobject"}};

        auto it = mimeTypes.find(fileExtension);
        return (it != mimeTypes.end()) ? it->second : "";
    }

}  // namespace http