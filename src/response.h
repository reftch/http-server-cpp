#ifndef HTTP_RESPONSE_H_
#define HTTP_RESPONSE_H_

#include <sys/socket.h>
#include <unistd.h>

#include <map>
#include <string>

#include "logger.h"

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
        SSE,
        UNKNOWN
    };

    enum class Status {
        // Information responses
        continue_ = 100,
        switching_protocol = 101,
        processing = 102,
        early_hints = 103,

        // Successful responses
        ok = 200,
        created = 201,
        accepted = 202,
        non_authoritative_information = 203,
        no_content = 204,
        reset_content = 205,
        partial_content = 206,
        multi_status = 207,
        already_reported = 208,

        // Redirection messages
        multiple_choices = 300,
        moved_permanently = 301,
        moved_temporarily = 302,
        not_modified = 304,
        use_proxy = 305,
        unused = 306,
        temporary_tedirect = 307,
        permanent_redirect = 308,

        // Client error responses
        bad_request = 400,
        unauthorized = 401,
        forbidden = 403,
        not_found = 404,
        method_not_allowed = 405,
        not_acceptable = 406,
        proxy_authentication_required = 407,
        request_timeout = 408,
        conflict = 409,
        gone = 410,
        length_required = 411,
        precondition_failed = 412,
        payload_too_large = 413,
        uri_too_long = 414,
        unsupported_media_type = 415,
        range_not_satisfiable = 416,
        expectation_failed = 417,
        im_A_teapot = 418,
        misdirected_request = 421,
        unprocessable_content = 422,
        locked = 423,
        failed_dependency = 424,
        too_early = 425,
        upgrade_required = 426,
        precondition_required = 428,
        too_many_requests = 429,
        request_header_fields_too_large = 431,
        unavailable_for_legal_reasons = 451,

        // Server error responses
        internal_server_error = 500,
        not_implemented = 501,
        bad_gateway = 502,
        service_unavailable = 503,
        gateway_timeout = 504,
        http_version_not_supported = 505,
        variant_also_negotiates = 506,
        insufficient_storage = 507,
        loop_detected = 508,
        not_extended = 510,
        network_authentication_required = 511
    };

    class Response {
       public:
        Response() { setHeader("Connection", "keep-alive"); };

        Response(const std::vector<std::pair<std::string, std::string>>& default_headers) {
            for (const auto& header : default_headers) {
                setHeader(header.first, header.second);
            }
        }

        Response(const std::vector<std::pair<std::string, std::string>>& default_headers, const int sockfd) {
            for (const auto& header : default_headers) {
                setHeader(header.first, header.second);
            }
            sockfd_ = sockfd;
        }

        void setHeader(const std::string& key, const std::string& val) { headers_[std::move(key)] = std::move(val); }

        std::string content() { return content_; }
        http::Status status() { return status_; }
        const std::map<std::string, std::string>& headers() const;

        template <ContentType T = ContentType::PLAIN_TEXT, Status S = Status::ok>
        Response& setContent(const std::string& content) {
            status_ = S;
            content_type_ = T;
            content_ = content;
            return *this;
        }

        // Overload << operator for ContentType
        Response& operator<<(ContentType type) {
            content_type_ = type;
            return *this;
        }

        // Overload << operator for Status
        Response& operator<<(Status status) {
            status_ = status;
            return *this;
        }

        // Overload << operator for string content
        Response& operator<<(const std::string& content) {
            content_ = content;
            return *this;
        }

        // Overload << operator for const char*
        Response& operator<<(const char* content) {
            content_ = content;
            return *this;
        }

        // Optional: overload for other types
        template <typename T>
        Response& operator<<(const T& value) {
            content_ = std::to_string(value);
            return *this;
        }

        std::string build(bool chunked = false);

        bool sendChunk() {
            std::string data = build(true);
            ssize_t written = ::send(sockfd_, data.data(), data.size(), MSG_NOSIGNAL);

            return written == (ssize_t)data.size();
        }

        void setStaticDirectory(const std::string static_directory) { static_directory_ = static_directory; }

       private:
        http::Status status_ = Status::ok;
        ContentType content_type_;
        std::string content_;
        std::string version_;
        std::string static_directory_;
        std::map<std::string, std::string> headers_;
        int sockfd_;
        Logger& log = Logger::getInstance();

        std::string statusToString();

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
                    return "text/javascript";
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
                case ContentType::SSE:
                    return "text/event-stream";
                case ContentType::UNKNOWN:
                    return "application/octet-stream";
            }

            // This should never be reached due to exhaustive switch
            return "application/octet-stream";
        }
    };

    namespace miscStrings {
        const char name_value_separator[] = {':', ' ', '\0'};
        const char crlf[] = {'\r', '\n', '\0'};
    }  // namespace miscStrings

    namespace statusToStrings {
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
    }  // namespace statusToStrings

}  // namespace http

#endif  // RESPONSE_HPP