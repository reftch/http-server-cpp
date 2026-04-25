#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <vector>

#include "header.hpp"

namespace http::server {

    namespace status {
        enum type {
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
    }

    namespace content_type {
        // API & Data Formats (Application Types)
        namespace api {
            constexpr const char* JSON = "application/json";
            constexpr const char* XML = "application/xml";
            constexpr const char* BINARY_STREAM = "application/octet-stream";  // Generic binary data/files
            constexpr const char* WEBASSEMBLY = "application/wasm";            // WebAssembly modules
        }  // namespace api

        // Text & Markup (HTML/CSS/JS)
        namespace text {
            constexpr const char* HTML = "text/html";
            constexpr const char* CSS = "text/css";
            constexpr const char* JavaScript = "application/javascript";  // Modern standard for JS
            constexpr const char* PLAIN_TEXT = "text/plain; charset=utf-8";
        }  // namespace text

        // Image & Graphics
        namespace image {
            constexpr const char* PNG = "image/png";
            constexpr const char* JPEG = "image/jpeg";
            constexpr const char* GIF = "image/gif";
            constexpr const char* SVG = "image/svg+xml";
        }  // namespace image

        // Documents & Files
        namespace documents {
            constexpr const char* PDF = "application/pdf";
            constexpr const char* TXT = "text/plain";  // General plain text files
        }  // namespace documents

        // Media (Audio & Video)
        namespace media {
            constexpr const char* MP3 = "audio/mpeg";
            constexpr const char* MP4 = "video/mp4";
            constexpr const char* WEBM = "video/webm";
        }  // namespace media

        // Fonts & Web Assets
        namespace web_assets {
            constexpr const char* WOFF2 = "font/woff2";
            constexpr const char* TTF = "font/ttf";
            constexpr const char* EOT = "font/ttf";  // Embedded OpenType
        }  // namespace web_assets

    }  // namespace content_type

    class response {
       public:
        // std::vector<header> getHeaders();

        response(const status::type& status, const char* type, const std::string& content);
        response(const status::type& status, const std::vector<header> headers, const std::string& content);

        std::string get_body();

       private:
        status::type status_;
        // The headers to be included in the reply.
        std::vector<header> headers_;
        // The content to be sent in the reply.
        std::string content_;

        std::string to_string();
    };

}  // namespace http::server

#endif  // RESPONSE_HPP