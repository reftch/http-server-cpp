#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <map>
#include <sstream>  // Required for std::stringstream
#include <string>
#include <unordered_map>

namespace http {

    /**
     * @brief Represents an HTTP request.
     *
     * This class encapsulates the components of an HTTP request such as method, path,
     * version, MIME type, path parameters, and query parameters.
     */
    class request {
       private:
        std::string method;                                   ///< The HTTP method (e.g., "GET", "POST").
        std::string path;                                     ///< The requested path (e.g., "/index.html").
        std::string version;                                  ///< The HTTP version (e.g., "HTTP/1.1").
        std::string mime_type;                                ///< The MIME type of the resource.
        std::unordered_map<std::string, std::string> params;  ///< Path parameters extracted from the URL.
        std::unordered_map<std::string, std::string> query;   ///< Query parameters from the URL.

       public:
        // Getters
        std::string getMethod() const {
            return method;
        }
        std::string getPath() const {
            return path;
        }
        std::string getMimeType() const {
            return mime_type;
        }
        std::unordered_map<std::string, std::string> getParams() const {
            return params;
        }
        std::unordered_map<std::string, std::string> getQuery() const {
            return query;
        }

        // Setters
        void setMethod(const std::string& m) {
            method = m;
        }
        void setPath(const std::string& p) {
            path = p;
        }
        void setVersion(const std::string& v) {
            version = v;
        }
        void setMimeType(const std::string& mt) {
            mime_type = mt;
        }

        void setParam(const std::string& key, const std::string& value) {
            params[key] = value;
        }

        void setQuery(const std::string& key, const std::string& value) {
            query[key] = value;
        }

        /**
         * @brief Default constructor.
         *
         * Initializes an empty HTTP request object.
         */
        request() = default;

        /**
         * @brief Constructs a request by parsing a raw HTTP request string.
         *
         * Parses the first line of the raw HTTP request string to extract:
         * - Method (e.g., "GET", "POST")
         * - Path (e.g., "/index.html")
         * - Version (e.g., "HTTP/1.1")
         *
         * @param raw_request The raw string containing the full HTTP request.
         */
        request(const std::string& raw_request);

        /**
         * @brief Determines the MIME type based on the file extension.
         *
         * Maps common file extensions to their corresponding MIME types.
         * For example:
         * - ".html" → "text/html"
         * - ".css"  → "text/css"
         * - ".js"   → "application/javascript"
         * - ".json" → "application/json"
         * - ".png"  → "image/png"
         *
         * If no matching extension is found, returns "application/octet-stream".
         *
         * @param path The file path or name to determine the MIME type for.
         * @return std::string The MIME type corresponding to the file extension.
         */
        std::string get_mime_type(const std::string& path);
    };

}  // namespace http

#endif  // REQUEST_HPP