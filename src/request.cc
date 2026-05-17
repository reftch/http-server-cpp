#include "request.h"

#include <iostream>
#include <string>

#include "response.h"
#include "router.h"

namespace http {

    Request::Request(const std::string& raw_request) {
        std::stringstream ss(raw_request);
        std::string line;

        if (std::getline(ss, line)) {
            ParseRequestLine(line);
            ParseHeaders(ss);
        }
    }

    bool Request::is_keep_alive() {
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

    void Request::ParseRequestLine(const std::string& line) {
        std::stringstream line_ss(line);
        std::string method, path, version;

        if (line_ss >> method >> path >> version) {
            this->method_ = method;
            this->path_ = path;
            this->version_ = version;
            this->mime_type_ = GetMimeType(path);
            this->params_ = {};
            this->query_ = {};
        }
    }

    void Request::ParseHeaders(std::istream& ss) {
        std::string line;
        while (std::getline(ss, line)) {
            if (line.empty()) {
                break;
            }

            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string header_name = line.substr(0, colon_pos);
                std::string header_value = line.substr(colon_pos + 1);
                headers_[header_name] = header_name;
            }
        }
    }

    std::string Request::GetMimeType(const std::string& path) {
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
            {"css", "text/css"},          {"js", "application/javascript"},
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