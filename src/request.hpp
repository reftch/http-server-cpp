#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <sstream>  // Required for std::stringstream
#include <string>

#include "request.hpp"

namespace http {

    namespace request {
        /**
         * @brief Structure to hold the essential components of an HTTP request line.
         */
        struct http_request {
            std::string method;   // The HTTP method (e.g., "GET", "POST").
            std::string path;     // The requested path (e.g., "/index.html").
            std::string version;  // The HTTP version (e.g., "HTTP/1.1").
        };

        /**
         * @brief Parses the first line of a raw HTTP request string to extract method, path, and version.
         *
         * This function assumes the request line is in the format: METHOD PATH VERSION (e.g., "GET /file HTTP/1.1").
         *
         * @param raw_request The raw string containing the HTTP request.
         * @return http_request A structure containing the extracted method, path, and version.
         */
        http_request parse(const std::string& raw_request);
    }  // namespace request

}  // namespace http
#endif  // REQUEST_HPP