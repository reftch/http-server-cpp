#include "response.hpp"

#include <unistd.h>

#include <string>

namespace http::server {
    //[status_strings Status Strings
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

    namespace misc_strings {
        const char name_value_separator[] = {':', ' ', '\0'};
        const char crlf[] = {'\r', '\n', '\0'};
    }  // namespace misc_strings

    response::response(const status::type& status, const char* type, const std::string& content)
        : status_(status), content_(content) {
        headers_.resize(3);
        headers_[0].name = "Content-Type";
        headers_[0].value = type;
        headers_[1].name = "Content-Length";
        headers_[1].value = std::to_string(content_.size());
        headers_[2].name = "Connection";
        headers_[2].value = "keep-alive";
    }

    response::response(const status::type& status, const std::vector<header> headers, const std::string& content)
        : status_(status), headers_(headers), content_(content) {}

    std::string response::get_body() {
        std::string body = to_string();
        for (std::size_t i = 0; i < headers_.size(); ++i) {
            header& h = headers_[i];
            body.append(h.name);
            body.append(misc_strings::name_value_separator);
            body.append(h.value);
            body.append(misc_strings::crlf);
        }
        body.append(misc_strings::crlf);
        body.append(content_);
        return body;
    }

    std::string response::to_string() {
        switch (status_) {
            case status::ok:
                return status_strings::ok;
            case status::created:
                return status_strings::created;
            case status::accepted:
                return status_strings::accepted;
            case status::no_content:
                return status_strings::no_content;
            case status::multiple_choices:
                return status_strings::multiple_choices;
            case status::moved_permanently:
                return status_strings::moved_permanently;
            case status::moved_temporarily:
                return status_strings::moved_temporarily;
            // case status_type::not_modified:
            //     return status_strings::not_modified;
            // case status_type::bad_request:
            //     return status_strings::bad_request;
            // case status_type::unauthorized:
            //     return status_strings::unauthorized;
            // case status_type::forbidden:
            //     return status_strings::forbidden;
            // case status_type::not_found:
            //     return status_strings::not_found;
            // case status_type::internal_server_error:
            //     return status_strings::internal_server_error;
            // case status_type::not_implemented:
            //     return status_strings::not_implemented;
            // case status_type::bad_gateway:
            //     return status_strings::bad_gateway;
            // case status_type::service_unavailable:
            //     return status_strings::service_unavailable;
            default:
                return status_strings::internal_server_error;
        }
    }

}  // namespace http::server