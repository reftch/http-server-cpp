#include "request.h"

#include <iostream>
#include <string>

#include "response.h"
#include "router.h"

namespace http {

    /*
    Request::Request(const std::string& raw_request) {
        // Use stringstream to easily read the input string line by line
        std::stringstream ss(raw_request);
        std::string line;

        // Attempt to read the first line of the request
        if (std::getline(ss, line)) {
            // Use another stringstream to parse the tokens on the extracted line
            std::stringstream line_ss(line);
            std::string method, path, version, connection;

            // Attempt to extract the three tokens (method, path, version) separated by spaces
            if (line_ss >> method >> path >> version) {
                this->method_ = method;
                this->path_ = path;
                this->version_ = version;
                this->mime_type_ = get_mime_type(path);
                this->params_ = {};
                this->query_ = {};

                // Parse headers
                while (std::getline(ss, line)) {
                    // Skip empty lines (end of headers)
                    if (line.empty()) {
                        break;
                    }

                    // Find the colon separator
                    size_t colon_pos = line.find(':');
                    if (colon_pos != std::string::npos) {
                        std::string header_name = line.substr(0, colon_pos);
                        std::string header_value = line.substr(colon_pos + 1);

                        // Trim whitespace from header value
                        header_value.erase(header_value.begin(),
                                           std::find_if(header_value.begin(), header_value.end(), [](unsigned char ch) {
                                               return !std::isspace(ch);
                                           }));
                        header_value.erase(std::find_if(header_value.rbegin(), header_value.rend(),
                                                        [](unsigned char ch) {
                                                            return !std::isspace(ch);
                                                        })
                                               .base(),
                                           header_value.end());

                        // Convert header name to lowercase for case-insensitive comparison
                        std::transform(header_name.begin(), header_name.end(), header_name.begin(), ::tolower);

                        // Parse specific headers
                        if (header_name == "connection") {
                            this->keep_alive_ = (header_value == "keep-alive" || header_value == "Keep-Alive");
                        } else if (header_name == "content-length") {
                            try {
                                this->content_length_ = std::stoul(header_value);
                            } catch (const std::exception&) {
                                this->content_length_ = 0;
                            }
                        } else if (header_name == "host") {
                            this->host_ = header_value;
                        } else if (header_name == "user-agent") {
                            this->user_agent_ = header_value;
                        }
                        // Add more headers as needed
                    }
                }
            }
        }
    }
*/

    Request::Request(const std::string& raw_request) {
        std::stringstream ss(raw_request);
        std::string line;

        if (std::getline(ss, line)) {
            parse_request_line(line);
            parse_headers(ss);
        }
    }

    void Request::parse_request_line(const std::string& line) {
        std::stringstream line_ss(line);
        std::string method, path, version;

        if (line_ss >> method >> path >> version) {
            this->method_ = method;
            this->path_ = path;
            this->version_ = version;
            this->mime_type_ = get_mime_type(path);
            this->params_ = {};
            this->query_ = {};
        }
    }

    void Request::parse_headers(std::istream& ss) {
        std::string line;
        while (std::getline(ss, line)) {
            if (line.empty()) {
                break;
            }

            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string header_name = line.substr(0, colon_pos);
                std::string header_value = line.substr(colon_pos + 1);

                // Trim whitespace from header value
                header_value.erase(header_value.begin(),
                                   std::find_if(header_value.begin(), header_value.end(), [](unsigned char ch) {
                                       return !std::isspace(ch);
                                   }));
                header_value.erase(std::find_if(header_value.rbegin(), header_value.rend(),
                                                [](unsigned char ch) {
                                                    return !std::isspace(ch);
                                                })
                                       .base(),
                                   header_value.end());

                // Convert header name to lowercase
                std::transform(header_name.begin(), header_name.end(), header_name.begin(), ::tolower);

                // Parse specific headers
                if (header_name == "connection") {
                    this->keep_alive_ = (header_value == "keep-alive" || header_value == "Keep-Alive");
                } else if (header_name == "content-length") {
                    try {
                        this->content_length_ = std::stoul(header_value);
                    } catch (const std::exception&) {
                        this->content_length_ = 0;
                    }
                } else if (header_name == "host") {
                    this->host_ = header_value;
                } else if (header_name == "user-agent") {
                    this->user_agent_ = header_value;
                }
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