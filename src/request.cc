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

    ContentType Request::GetMimeType(const std::string& path) {
        // Find base path (up to '?' or whole string if no '?')
        std::string_view root{path};
        size_t query_pos = path.find('?');
        if (query_pos != std::string::npos) {
            root = std::string_view{path}.substr(0, query_pos);
        }

        // Find last dot; skip if missing
        size_t dot_pos = root.find_last_of('.');
        if (dot_pos == std::string_view::npos) {
            return ContentType::UNKNOWN;
        }

        // Extract extension (as string_view, no allocation)
        std::string_view ext = root.substr(dot_pos + 1);

        // Map file extensions to ContentType enum
        static constexpr std::pair<std::string_view, ContentType> mimeTypes[] = {
            {"html", ContentType::HTML},      {"htm", ContentType::HTML},    {"css", ContentType::CSS},
            {"js", ContentType::JAVASCRIPT},  {"jpg", ContentType::JPEG},    {"jpeg", ContentType::JPEG},
            {"png", ContentType::PNG},        {"xml", ContentType::XML},     {"json", ContentType::JSON},
            {"txt", ContentType::PLAIN_TEXT}, {"gif", ContentType::GIF},     {"svg", ContentType::SVG},
            {"pdf", ContentType::PDF},        {"mp3", ContentType::MP3},     {"mp4", ContentType::MP4},
            {"webm", ContentType::WEBM},      {"woff2", ContentType::WOFF2}, {"ttf", ContentType::TTF},
            {"eot", ContentType::EOT}};

        // Convert extension to lowercase in a small buffer (no new string)
        char buf[16];
        size_t len = std::min(ext.size(), sizeof(buf) - 1);
        std::transform(ext.begin(), ext.begin() + len, buf, ::tolower);
        buf[len] = '\0';
        std::string_view lower{buf, len};

        // Linear search; very fast for small, hot lists
        for (const auto& [k, v] : mimeTypes) {
            if (k == lower) {
                return v;
            }
        }

        return ContentType::UNKNOWN;
    }

}  // namespace http