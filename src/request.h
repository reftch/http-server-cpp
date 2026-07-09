#ifndef HTTP_REQUEST_H_
#define HTTP_REQUEST_H_

#include <optional>
#include <string>
#include <unordered_map>

namespace http {

    /**
     * @brief Represents an HTTP request with all its components
     *
     * This class parses and stores HTTP request data including method, path,
     * headers, query parameters, and body. It provides methods for accessing
     * and manipulating request properties.
     */
    class Request {
       public:
        /**
         * @brief Default constructor
         */
        Request() = default;

        /**
         * @brief Construct request from raw HTTP request string
         * @param raw_request Complete raw HTTP request string
         */
        explicit Request(const std::string& raw_request);

        /**
         * @brief Get HTTP method (GET, POST, etc.)
         * @return HTTP method string
         */
        const std::string& method() const { return method_; }

        /**
         * @brief Get request path
         * @return Path string
         */
        const std::string& path() const { return path_; }

        /**
         * @brief Get normalized path
         * @return Normalized path string
         */
        const std::string& normalize_path() const { return normalize_path_; }

        /**
         * @brief Get query string from URL
         * @return Query string
         */
        const std::string& query_string() const { return query_string_; }

        /**
         * @brief Get HTTP version
         * @return Version string
         */
        const std::string& version() const { return version_; }

        /**
         * @brief Get request body
         * @return Body string
         */
        const std::string& body() const { return body_; }

        /**
         * @brief Get headers map
         * @return Map of header key-value pairs
         */
        const std::unordered_map<std::string_view, std::string_view>& headers() const { return headers_; }

        /**
         * @brief Get path parameters
         * @return Map of parameter key-value pairs
         */
        const std::unordered_map<std::string, std::string>& params() const { return params_; }

        /**
         * @brief Get query parameters
         * @return Map of query key-value pairs
         */
        const std::unordered_map<std::string, std::string>& query() const { return query_; }

        /**
         * @brief Set HTTP method
         * @param method The HTTP method to set
         */
        void setMethod(const std::string& method) { method_ = method; }

        /**
         * @brief Set request path
         * @param path The path to set
         */
        void setPath(const std::string& path) { path_ = path; }

        /**
         * @brief Set HTTP version
         * @param version The version string to set
         */
        void setVersion(const std::string& version) { version_ = version; }

        /**
         * @brief Set MIME type for the request
         * @param mime_type The MIME type to set
         */
        void setMimeType(const std::string& mime_type) { mime_type_ = mime_type; }

        /**
         * @brief Set path parameter
         * @param key Parameter key
         * @param value Parameter value
         */
        void setParam(const std::string& key, const std::string& value) { params_[key] = value; }

        /**
         * @brief Set query parameter
         * @param key Parameter key
         * @param value Parameter value
         */
        void setQuery(const std::string& key, const std::string& value) { query_[key] = value; }

        /**
         * @brief Check if connection should be kept alive
         * @return True if keep-alive is requested, false otherwise
         */
        bool isKeepAlive();

        /**
         * @brief Get MIME type of the request
         * @return Optional MIME type string, nullopt if not set
         */
        std::optional<std::string> mimeType() const {
            if (mime_type_.empty()) {
                return std::nullopt;
            }
            return mime_type_;
        }

        /**
         * @brief Get MIME type based on file path extension
         * @param path File path to determine MIME type for
         * @return MIME type string
         */
        std::string getMimeType(const std::string& path);

       private:
        std::string method_;
        std::string path_;
        std::string normalize_path_;
        std::string query_string_;
        std::string version_;
        std::string mime_type_;
        std::string body_;

        std::unordered_map<std::string_view, std::string_view> headers_;
        std::unordered_map<std::string, std::string> params_;
        std::unordered_map<std::string, std::string> query_;

        /**
         * @brief Parse HTTP request line (method, path, version)
         * @param line Request line to parse
         */
        void parseRequestLine(std::string_view line);

        /**
         * @brief Parse headers from raw request string
         * @param raw_request Complete raw request string
         * @param pos Current position in the string
         */
        void parseHeaders(std::string_view raw_request, size_t& pos);

        /**
         * @brief Parse query string into key-value pairs
         * @param query_string Query string to parse
         */
        void parseQueryString(const std::string& query_string);
    };

}  // namespace http

#endif  // REQUEST_HPP