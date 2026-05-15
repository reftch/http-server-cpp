#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <map>
#include <sstream>  // Required for std::stringstream
#include <string>
#include <unordered_map>

#include "request.hpp"

namespace http {

    class request {
       private:
        std::string method;                                   // The HTTP method (e.g., "GET", "POST").
        std::string path;                                     // The requested path (e.g., "/index.html").
        std::string version;                                  // The HTTP version (e.g., "HTTP/1.1").
        std::string mime_type;                                // The Mime Type
        std::unordered_map<std::string, std::string> params;  // Path parameters
        std::unordered_map<std::string, std::string> query;   // Query parameters

       public:
        std::string getMethod() {
            return method;
        }
        std::string getPath() {
            return path;
        }
        std::string getMimeType() {
            return mime_type;
        }

        void setParam(const std::string& key, const std::string& value) {
            params[key] = value;
        }

        std::unordered_map<std::string, std::string> getParams() {
            return params;
        }

        void setQuery(const std::string& key, const std::string& value) {
            query[key] = value;
        }

        std::unordered_map<std::string, std::string> getQuery() {
            return query;
        }

        /**
         * @brief Parses the first line of a raw HTTP request string to extract method, path, and version.
         *
         * This function assumes the request line is in the format: METHOD PATH VERSION (e.g., "GET /file
         HTTP/1.1").
         *
         * @param raw_request The raw string containing the HTTP request.
         * @return context A structure containing the extracted method, path, and version.
         */
        request(const std::string& raw_request);

        /**
         * @brief Determines the MIME type based on the file extension.
         *
         * This function maps common file extensions to their corresponding MIME types.
         * For example, ".html" maps to "text/html", ".css" maps to "text/css", etc.
         *
         * @param path The file path or name to determine the MIME type for.
         * @return std::string The MIME type corresponding to the file extension.
         */
        std::string get_mime_type(const std::string& path);
    };

}  // namespace http
#endif  // REQUEST_HPP