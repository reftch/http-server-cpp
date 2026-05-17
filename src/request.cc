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

    // std::string Request::get_mime_type(const std::string& path) {
    //     // Extract just the filename part (before any query parameters)
    //     std::string clean_path = path;
    //     size_t query_pos = path.find('?');
    //     if (query_pos != std::string::npos) {
    //         clean_path = path.substr(0, query_pos);
    //     }

    //     static const std::map<std::string, const char*> mimeTypes = {
    //         {"html", content_type::HTML}, {"css", content_type::CSS},        {"js", content_type::JavaScript},
    //         {"jpg", content_type::JPEG},  {"png", content_type::PNG},        {"xml", content_type::XML},
    //         {"json", content_type::JSON}, {"txt", content_type::PLAIN_TEXT}, {"gif", content_type::GIF},
    //         {"svg", content_type::SVG},   {"pdf", content_type::PDF},        {"mp3", content_type::MP3},
    //         {"mp4", content_type::MP4},   {"webm", content_type::WEBM},      {"woff2", content_type::WOFF2},
    //         {"ttf", content_type::TTF},   {"eot", content_type::EOT}};

    //     std::string fileExtension = clean_path.substr(clean_path.find_last_of(".") + 1);
    //     auto it = mimeTypes.find(fileExtension);
    //     return (it != mimeTypes.end()) ? it->second : "";
    // }

    ContentType Request::GetMimeType(const std::string& path) {
        // Extract just the filename part (before any query parameters)
        std::string clean_path = path;
        size_t query_pos = path.find('?');
        if (query_pos != std::string::npos) {
            clean_path = path.substr(0, query_pos);
        }

        // Map file extensions to ContentType enum
        static const std::map<std::string, ContentType> mimeTypes = {
            {"html", ContentType::HTML},      {"htm", ContentType::HTML},    {"css", ContentType::CSS},
            {"js", ContentType::JAVASCRIPT},  {"jpg", ContentType::JPEG},    {"jpeg", ContentType::JPEG},
            {"png", ContentType::PNG},        {"xml", ContentType::XML},     {"json", ContentType::JSON},
            {"txt", ContentType::PLAIN_TEXT}, {"gif", ContentType::GIF},     {"svg", ContentType::SVG},
            {"pdf", ContentType::PDF},        {"mp3", ContentType::MP3},     {"mp4", ContentType::MP4},
            {"webm", ContentType::WEBM},      {"woff2", ContentType::WOFF2}, {"ttf", ContentType::TTF},
            {"eot", ContentType::EOT}};

        // Extract file extension
        size_t dot_pos = clean_path.find_last_of(".");
        if (dot_pos == std::string::npos) {
            return ContentType::UNKNOWN;  // Default fallback
        }

        std::string fileExtension = clean_path.substr(dot_pos + 1);

        // Convert to lowercase for case-insensitive matching
        std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);

        auto it = mimeTypes.find(fileExtension);
        if (it != mimeTypes.end()) {
            return it->second;
        }

        return ContentType::UNKNOWN;  // Default fallback
    }

}  // namespace http