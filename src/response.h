#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <vector>

#include "header.h"

namespace http {

    enum Status {
        ok = 200,
        created = 201,
        accepted = 202,
        no_content = 204,
        multiple_choices = 300,
        moved_permanently = 301,
        moved_temporarily = 302,
        not_modified = 304,
        bad_request = 400,
        unauthorized = 401,
        forbidden = 403,
        not_found = 404,
        internal_server_error = 500,
        not_implemented = 501,
        bad_gateway = 502,
        service_unavailable = 503
    };

    class Response {
       public:
        Response() = default;

        void set_status(Status status) {
            status_ = status;
        }

        void set_version(const std::string& version) {
            version_ = version;
        }

        void set_header(const std::string& key, const std::string& val) {
            // Check if header already exists and update it
            for (auto& header : headers_) {
                if (header.key == key) {
                    header.value = val;
                    return;
                }
            }
            // If header doesn't exist, add it
            headers_.emplace_back(key, val);
        }

        void set_content(const std::string& s, const std::string& content_type);
        void set_content(const Status& status, const std::string& content, const std::string& content_type);

        std::string build();

       private:
        http::Status status_ = ok;
        std::string version_;
        std::string content_;
        std::string content_type_;
        std::vector<Header> headers_;

        std::string status();
    };

    namespace misc_strings {
        const char name_value_separator[] = {':', ' ', '\0'};
        const char crlf[] = {'\r', '\n', '\0'};
    }  // namespace misc_strings

    namespace content_type {
        constexpr const char* JSON = "application/json";
        constexpr const char* XML = "application/xml";
        constexpr const char* BINARY_STREAM = "application/octet-stream";  // Generic binary data/files
        constexpr const char* WEBASSEMBLY = "application/wasm";            // WebAssembly modules
        constexpr const char* HTML = "text/html";
        constexpr const char* CSS = "text/css";
        constexpr const char* JavaScript = "application/javascript";  // Modern standard for JS
        constexpr const char* PLAIN_TEXT = "text/plain; charset=utf-8";
        constexpr const char* PNG = "image/png";
        constexpr const char* JPEG = "image/jpeg";
        constexpr const char* GIF = "image/gif";
        constexpr const char* SVG = "image/svg+xml";
        constexpr const char* PDF = "application/pdf";
        constexpr const char* TXT = "text/plain";  // General plain text files
        constexpr const char* MP3 = "audio/mpeg";
        constexpr const char* MP4 = "video/mp4";
        constexpr const char* WEBM = "video/webm";
        constexpr const char* WOFF2 = "font/woff2";
        constexpr const char* TTF = "font/ttf";
        constexpr const char* EOT = "font/ttf";  // Embedded OpenType
    }  // namespace content_type

    namespace status_strings {
        const std::string ok = "HTTP/1.0 200 OK\r\n";
        const std::string created = "HTTP/1.0 201 Created\r\n";
        const std::string accepted = "HTTP/1.0 202 Accepted\r\n";
        const std::string no_content = "HTTP/1.0 204 No Content\r\n";
        const std::string multiple_choices = "HTTP/1.0 300 Multiple Choices\r\n";
        const std::string moved_permanently = "HTTP/1.0 301 Moved Permanently\r\n";
        const std::string moved_temporarily = "HTTP/1.0 302 Moved Temporarily\r\n";
        const std::string not_modified = "HTTP/1.0 304 Not Modified\r\n";
        const std::string bad_request = "HTTP/1.0 400 Bad Request\r\n";
        const std::string unauthorized = "HTTP/1.0 401 Unauthorized\r\n";
        const std::string forbidden = "HTTP/1.0 403 Forbidden\r\n";
        const std::string not_found = "HTTP/1.0 404 Not Found\r\n";
        const std::string internal_server_error = "HTTP/1.0 500 Internal Server Error\r\n";
        const std::string not_implemented = "HTTP/1.0 501 Not Implemented\r\n";
        const std::string bad_gateway = "HTTP/1.0 502 Bad Gateway\r\n";
        const std::string service_unavailable = "HTTP/1.0 503 Service Unavailable\r\n";
    }  // namespace status_strings

    // namespace response {
    //     enum status {
    //         ok = 200,
    //         created = 201,
    //         accepted = 202,
    //         no_content = 204,
    //         multiple_choices = 300,
    //         moved_permanently = 301,
    //         moved_temporarily = 302,
    //         not_modified = 304,
    //         bad_request = 400,
    //         unauthorized = 401,
    //         forbidden = 403,
    //         not_found = 404,
    //         internal_server_error = 500,
    //         not_implemented = 501,
    //         bad_gateway = 502,
    //         service_unavailable = 503
    //     };

    //     namespace content_type {
    //         constexpr const char* JSON = "application/json";
    //         constexpr const char* XML = "application/xml";
    //         constexpr const char* BINARY_STREAM = "application/octet-stream";  // Generic binary data/files
    //         constexpr const char* WEBASSEMBLY = "application/wasm";            // WebAssembly modules
    //         constexpr const char* HTML = "text/html";
    //         constexpr const char* CSS = "text/css";
    //         constexpr const char* JavaScript = "application/javascript";  // Modern standard for JS
    //         constexpr const char* PLAIN_TEXT = "text/plain; charset=utf-8";
    //         constexpr const char* PNG = "image/png";
    //         constexpr const char* JPEG = "image/jpeg";
    //         constexpr const char* GIF = "image/gif";
    //         constexpr const char* SVG = "image/svg+xml";
    //         constexpr const char* PDF = "application/pdf";
    //         constexpr const char* TXT = "text/plain";  // General plain text files
    //         constexpr const char* MP3 = "audio/mpeg";
    //         constexpr const char* MP4 = "video/mp4";
    //         constexpr const char* WEBM = "video/webm";
    //         constexpr const char* WOFF2 = "font/woff2";
    //         constexpr const char* TTF = "font/ttf";
    //         constexpr const char* EOT = "font/ttf";  // Embedded OpenType
    //     }  // namespace content_type

    //     namespace status_strings {
    //         const std::string ok = "HTTP/1.0 200 OK\r\n";
    //         const std::string created = "HTTP/1.0 201 Created\r\n";
    //         const std::string accepted = "HTTP/1.0 202 Accepted\r\n";
    //         const std::string no_content = "HTTP/1.0 204 No Content\r\n";
    //         const std::string multiple_choices = "HTTP/1.0 300 Multiple Choices\r\n";
    //         const std::string moved_permanently = "HTTP/1.0 301 Moved Permanently\r\n";
    //         const std::string moved_temporarily = "HTTP/1.0 302 Moved Temporarily\r\n";
    //         const std::string not_modified = "HTTP/1.0 304 Not Modified\r\n";
    //         const std::string bad_request = "HTTP/1.0 400 Bad Request\r\n";
    //         const std::string unauthorized = "HTTP/1.0 401 Unauthorized\r\n";
    //         const std::string forbidden = "HTTP/1.0 403 Forbidden\r\n";
    //         const std::string not_found = "HTTP/1.0 404 Not Found\r\n";
    //         const std::string internal_server_error = "HTTP/1.0 500 Internal Server Error\r\n";
    //         const std::string not_implemented = "HTTP/1.0 501 Not Implemented\r\n";
    //         const std::string bad_gateway = "HTTP/1.0 502 Bad Gateway\r\n";
    //         const std::string service_unavailable = "HTTP/1.0 503 Service Unavailable\r\n";
    //     }  // namespace status_strings

    //     namespace misc_strings {
    //         const char name_value_separator[] = {':', ' ', '\0'};
    //         const char crlf[] = {'\r', '\n', '\0'};
    //     }  // namespace misc_strings

    //     std::string create(const char* type, const std::string& content);
    //     std::string create(const status& status_type, const char* type, const std::string& content);
    //     std::string json(const std::string& content);
    //     std::string html(const std::string& content);
    //     std::string to_string(const status& status);

    // }  // namespace response

}  // namespace http

#endif  // RESPONSE_HPP