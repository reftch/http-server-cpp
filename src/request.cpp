#include "request.hpp"

#include <iostream>
#include <string>

#include "response.hpp"
#include "router.hpp"

namespace http {

    /**
     * @brief Parses the first line of a raw HTTP request string to extract method, path, and version.
     *
     * This function assumes the request line is in the format: METHOD PATH VERSION (e.g., "GET /file HTTP/1.1").
     *
     * @param raw_request The raw string containing the HTTP request.
     * @return http_request A structure containing the extracted method, path, and version.
     */
    request::request(const std::string& raw_request) {
        // context ctx;

        // Use stringstream to easily read the input string line by line
        std::stringstream ss(raw_request);
        std::string line;

        // Attempt to read the first line of the request
        if (std::getline(ss, line)) {
            // Use another stringstream to parse the tokens on the extracted line
            std::stringstream line_ss(line);
            std::string method, path, version;

            // Attempt to extract the three tokens (method, path, version) separated by spaces
            if (line_ss >> method >> path >> version) {
                this->method = method;
                this->path = path;
                this->version = version;
                this->mime_type = get_mime_type(path);
                this->params = {};
                this->query = {};
            }
            // Note: If the format is incorrect, req remains default-initialized.
        }

        // return ctx;
    }

    std::string request::get_mime_type(const std::string& path) {
        // Extract just the filename part (before any query parameters)
        std::string clean_path = path;
        size_t query_pos = path.find('?');
        if (query_pos != std::string::npos) {
            clean_path = path.substr(0, query_pos);
        }

        static const std::map<std::string, const char*> mimeTypes = {
            {"html", response::content_type::HTML},     {"css", response::content_type::CSS},
            {"js", response::content_type::JavaScript}, {"jpg", response::content_type::JPEG},
            {"png", response::content_type::PNG},       {"xml", response::content_type::XML},
            {"json", response::content_type::JSON},     {"txt", response::content_type::PLAIN_TEXT},
            {"gif", response::content_type::GIF},       {"svg", response::content_type::SVG},
            {"pdf", response::content_type::PDF},       {"mp3", response::content_type::MP3},
            {"mp4", response::content_type::MP4},       {"webm", response::content_type::WEBM},
            {"woff2", response::content_type::WOFF2},   {"ttf", response::content_type::TTF},
            {"eot", response::content_type::EOT}};

        std::string fileExtension = clean_path.substr(clean_path.find_last_of(".") + 1);
        auto it = mimeTypes.find(fileExtension);
        return (it != mimeTypes.end()) ? it->second : "";
    }

}  // namespace http