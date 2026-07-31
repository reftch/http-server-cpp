#ifndef HTTP_RESPONSE_H_
#define HTTP_RESPONSE_H_

#include <sys/socket.h>
#include <unistd.h>

#include <map>
#include <string>

#ifdef HTTP_OPENSSL_SUPPORT
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

#include "logger.h"
#include "status.h"

namespace http {

    /**
     * @brief Enum representing common MIME content types.
     */
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
        SSE,  // Server-Sent Events
        UNKNOWN
    };

    /**
     * @brief A class to construct and send HTTP responses.
     *
     * This class supports setting headers, status code, content type, and content.
     * It also provides functionality for sending response chunks over a socket or SSL connection.
     */
    class Response {
       public:
        /**
         * @brief Constructs an empty HTTP response with default "Connection: keep-alive".
         */
        Response() { setHeader("Connection", "keep-alive"); };

        /**
         * @brief Constructs a response with headers and a socket file descriptor.
         * @param default_headers Initial list of header key-value pairs.
         * @param sockfd Socket file descriptor used to send the response.
         */
        Response(const std::vector<std::pair<std::string, std::string>>& default_headers, const int sockfd) {
            for (const auto& header : default_headers) {
                setHeader(header.first, header.second);
            }
            sockfd_ = sockfd;
            setHeader("Connection", "keep-alive");
        }

#ifdef HTTP_OPENSSL_SUPPORT
        /**
         * @brief Constructs a response with headers and an SSL context.
         * @param default_headers Initial list of header key-value pairs.
         * @param ssl OpenSSL SSL structure used to send the response.
         */
        Response(const std::vector<std::pair<std::string, std::string>>& default_headers, SSL* ssl) : ssl(ssl) {
            for (const auto& header : default_headers) {
                setHeader(header.first, header.second);
            }
            setHeader("Connection", "keep-alive");
        }
#endif

        /**
         * @brief Sets a custom header in the response.
         * @param key Header name.
         * @param val Header value.
         */
        void setHeader(const std::string& key, const std::string& val) { headers_[std::move(key)] = std::move(val); }

        /**
         * @brief Gets the content of the response body.
         * @return The current content string.
         */
        std::string content() { return content_; }

        /**
         * @brief Gets the status code of the response.
         * @return The current HTTP status.
         */
        http::Status status() { return status_; }

        /**
         * @brief Gets all headers as a map.
         * @return Reference to internal headers map.
         */
        const std::map<std::string, std::string>& headers() const;

        /**
         * @brief Sets the content with type and status using template parameters.
         * @tparam T Content type (default: PLAIN_TEXT).
         * @tparam S Status code (default: OK).
         * @param content The response body content.
         * @return Reference to this object for chaining.
         */
        template <ContentType T = ContentType::PLAIN_TEXT, Status S = Status::ok>
        Response& setContent(const std::string& content) {
            status_ = S;
            content_type_ = T;
            content_ = content;
            return *this;
        }

        /**
         * @brief Sets the content type via operator<<.
         * @param type Content type to set.
         * @return Reference to this object for chaining.
         */
        Response& operator<<(ContentType type) {
            content_type_ = type;
            return *this;
        }

        /**
         * @brief Sets the status code via operator<<.
         * @param status Status code to set.
         * @return Reference to this object for chaining.
         */
        Response& operator<<(Status status) {
            status_ = status;
            return *this;
        }

        /**
         * @brief Appends string content via operator<<.
         * @param content Content to append.
         * @return Reference to this object for chaining.
         */
        Response& operator<<(const std::string& content) {
            content_ += content;
            return *this;
        }

        /**
         * @brief Appends C-style string content via operator<<.
         * @param content Content to append.
         * @return Reference to this object for chaining.
         */
        Response& operator<<(const char* content) {
            content_ += content;
            return *this;
        }

        /**
         * @brief Converts a value to string and appends it via operator<<.
         * @tparam T Type of the value.
         * @param value Value to convert and append.
         * @return Reference to this object for chaining.
         */
        template <typename T>
        Response& operator<<(const T& value) {
            content_ += std::to_string(value);
            return *this;
        }

        /**
         * @brief Builds the full HTTP response string.
         * @param chunked Whether to build a chunked transfer encoding response.
         * @return Full HTTP response as a string.
         */
        std::string build(bool chunked = false);

        /**
         * @brief Sends the response as a chunk over socket or SSL connection.
         * @return True if all bytes were sent successfully, otherwise false.
         */
        bool sendChunk() {
            const auto data = build(true);
            const auto expected_bytes = static_cast<ssize_t>(data.size());

#ifdef HTTP_OPENSSL_SUPPORT
            const auto written = SSL_write(ssl, data.data(), data.size());
#else
            const auto written = ::send(sockfd_, data.data(), data.size(), MSG_NOSIGNAL);
#endif

            return written == expected_bytes;
        }

        /**
         * @brief Sets the static directory used for serving files.
         * @param static_directory Directory path to serve static assets from.
         */
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

#ifdef HTTP_OPENSSL_SUPPORT
        SSL* ssl = nullptr;
#endif

        /**
         * @brief Converts a status code to its corresponding HTTP response line.
         * @return String representation of the status line.
         */
        std::string statusToString();

        /**
         * @brief Converts a content type enum into its MIME string equivalent.
         * @param type Content type to convert.
         * @return Corresponding MIME type string.
         */
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

    /**
     * @brief Namespace containing various helper strings used in HTTP formatting.
     */
    namespace miscStrings {
        const char name_value_separator[] = {':', ' ', '\0'};
        const char crlf[] = {'\r', '\n', '\0'};
    }  // namespace miscStrings

}  // namespace http

#endif  // HTTP_RESPONSE_H_