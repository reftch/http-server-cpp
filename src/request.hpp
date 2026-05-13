#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <map>
#include <sstream>  // Required for std::stringstream
#include <string>
#include <unordered_map>

#include "request.hpp"

namespace http {

    namespace request {
        /**
         * @brief Structure to hold the essential components of an HTTP request line.
         */
        struct context {
            std::string method;                                   // The HTTP method (e.g., "GET", "POST").
            std::string path;                                     // The requested path (e.g., "/index.html").
            std::string version;                                  // The HTTP version (e.g., "HTTP/1.1").
            std::string mime_type;                                // The Mime Type
            std::unordered_map<std::string, std::string> params;  // Path parameters
            std::unordered_map<std::string, std::string> query;   // Query parameters
        };

        /**
         * @brief Parses the first line of a raw HTTP request string to extract method, path, and version.
         *
         * This function assumes the request line is in the format: METHOD PATH VERSION (e.g., "GET /file HTTP/1.1").
         *
         * @param raw_request The raw string containing the HTTP request.
         * @return http_request A structure containing the extracted method, path, and version.
         */
        context parse(const std::string& raw_request);

        std::string get_mime_type(const std::string& path);
    }  // namespace request

}  // namespace http
#endif  // REQUEST_HPP