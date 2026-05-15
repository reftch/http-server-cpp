#include "request.h"

#include <iostream>
#include <string>

#include "response.h"
#include "router.h"

namespace http {

    Request::Request(const std::string& raw_request) {
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
                this->method_ = method;
                this->path_ = path;
                this->version_ = version;
                this->mime_type_ = get_mime_type(path);
                this->params_ = {};
                this->query_ = {};
            }
        }
    }

    std::string Request::get_mime_type(const std::string& path) {
        // Extract just the filename part (before any query parameters)
        std::string clean_path = path;
        size_t query_pos = path.find('?');
        if (query_pos != std::string::npos) {
            clean_path = path.substr(0, query_pos);
        }

        static const std::map<std::string, const char*> mimeTypes = {
            {"html", content_type::HTML}, {"css", content_type::CSS},        {"js", content_type::JavaScript},
            {"jpg", content_type::JPEG},  {"png", content_type::PNG},        {"xml", content_type::XML},
            {"json", content_type::JSON}, {"txt", content_type::PLAIN_TEXT}, {"gif", content_type::GIF},
            {"svg", content_type::SVG},   {"pdf", content_type::PDF},        {"mp3", content_type::MP3},
            {"mp4", content_type::MP4},   {"webm", content_type::WEBM},      {"woff2", content_type::WOFF2},
            {"ttf", content_type::TTF},   {"eot", content_type::EOT}};

        std::string fileExtension = clean_path.substr(clean_path.find_last_of(".") + 1);
        auto it = mimeTypes.find(fileExtension);
        return (it != mimeTypes.end()) ? it->second : "";
    }

}  // namespace http