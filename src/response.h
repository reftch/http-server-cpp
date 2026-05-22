#ifndef HTTP_RESPONSE_H_
#define HTTP_RESPONSE_H_

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "utils.h"

namespace http {

    enum class ContentType {
        HTML,
        CSS,
        JAVASCRIPT,
        JPEG,
        PNG,
        XML,
        JSON,
        PLAIN_TEXT,
        GIF,
        SVG,
        PDF,
        MP3,
        MP4,
        WEBM,
        WOFF2,
        TTF,
        EOT,
        UNKNOWN
    };

    enum class Status {
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
        Response(bool is_keep_alive, const std::string& static_directory);

        void set_header(const std::string& key, const std::string& val) { headers_[key] = val; }

        std::string content() { return content_; }
        http::Status status() { return status_; }
        std::unordered_map<std::string, std::string> headers() { return headers_; }

        template <ContentType T = ContentType::PLAIN_TEXT, Status S = Status::ok>
        void SetContent(const std::string& content) {
            set_header("Content-Type", ContentTypeToString(T));

            // Set content based on content type
            if (T == ContentType::HTML) {
                content_ = ReadFile(static_directory_ + '/' + content);
            } else {
                content_ = content;
            }

            // Set Content-Length header using the actual content size
            set_header("Content-Length", std::to_string(content_.size()));

            status_ = S;
        }

        template <Status S = Status::ok>
        void SetContentByType(const std::string& content, std::string type) {
            set_header("Content-Type", type);
            set_header("Content-Length", std::to_string(content.size()));
            content_ = content;
            status_ = S;
        }

        void SetContentByType(const std::string& content, Status s = Status::ok) {
            // set_header("Content-Type", type);
            set_header("Content-Length", std::to_string(content.size()));
            content_ = content;
            status_ = s;
        }

        // void SetContent(const std::string& content) { content_ = content; }

        std::string Build();

       private:
        http::Status status_ = Status::ok;
        std::string version_;
        std::string content_;
        std::string content_type_;
        std::string static_directory_;
        std::unordered_map<std::string, std::string> headers_;

        std::string StatusToString();

        constexpr std::string ContentTypeToString(ContentType type) {
            switch (type) {
                case ContentType::JSON:
                    return "application/json";
                case ContentType::HTML:
                    return "text/html";
                case ContentType::PLAIN_TEXT:
                    return "text/plain; charset=utf-8";
                case ContentType::CSS:
                    return "text/css";
                case ContentType::JAVASCRIPT:
                    return "application/javascript";
                case ContentType::JPEG:
                    return "image/jpeg";
                case ContentType::PNG:
                    return "image/png";
                case ContentType::GIF:
                    return "image/gif";
                case ContentType::SVG:
                    return "image/svg+xml";
                case ContentType::PDF:
                    return "application/pdf";
                case ContentType::MP3:
                    return "audio/mpeg";
                case ContentType::MP4:
                    return "video/mp4";
                case ContentType::WEBM:
                    return "video/webm";
                case ContentType::WOFF2:
                    return "font/woff2";
                case ContentType::TTF:
                    return "font/ttf";
                case ContentType::EOT:
                    return "application/vnd.ms-fontobject";
                case ContentType::XML:
                    return "application/xml";
                case ContentType::UNKNOWN:
                    return "application/octet-stream";
            }

            // This should never be reached due to exhaustive switch
            return "application/octet-stream";
        }
    };

    namespace misc_strings {
        const char name_value_separator[] = {':', ' ', '\0'};
        const char crlf[] = {'\r', '\n', '\0'};
    }  // namespace misc_strings

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

}  // namespace http

#endif  // RESPONSE_HPP