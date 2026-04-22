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

    namespace stock {
        response ok(std::string message) {
            response resp;
            resp.status = response::status_type::ok;
            resp.headers.resize(3);
            resp.headers[0].name = "Content-Type";
            resp.headers[0].value = "text/plain; charset=utf-8";
            resp.headers[1].name = "Content-Length";
            resp.content = message;
            resp.headers[1].value = std::to_string(resp.content.size());
            resp.headers[2].name = "Connection";
            resp.headers[2].value = "keep-alive";
            return resp;
        }

        std::string to_string(response resp) {
            std::string body = to_string(resp.status);
            for (std::size_t i = 0; i < resp.headers.size(); ++i) {
                header& h = resp.headers[i];
                body.append(h.name);
                body.append(misc_strings::name_value_separator);
                body.append(h.value);
                body.append(misc_strings::crlf);
            }
            body.append(misc_strings::crlf);
            body.append(resp.content);
            return body;
        }

        std::string to_string(response::status_type status) {
            switch (status) {
                case response::ok:
                    return status_strings::ok;
                case response::created:
                    return status_strings::created;
                case response::accepted:
                    return status_strings::accepted;
                case response::no_content:
                    return status_strings::no_content;
                case response::multiple_choices:
                    return status_strings::multiple_choices;
                case response::moved_permanently:
                    return status_strings::moved_permanently;
                case response::moved_temporarily:
                    return status_strings::moved_temporarily;
                case response::not_modified:
                    return status_strings::not_modified;
                case response::bad_request:
                    return status_strings::bad_request;
                case response::unauthorized:
                    return status_strings::unauthorized;
                case response::forbidden:
                    return status_strings::forbidden;
                case response::not_found:
                    return status_strings::not_found;
                case response::internal_server_error:
                    return status_strings::internal_server_error;
                case response::not_implemented:
                    return status_strings::not_implemented;
                case response::bad_gateway:
                    return status_strings::bad_gateway;
                case response::service_unavailable:
                    return status_strings::service_unavailable;
                default:
                    return status_strings::internal_server_error;
            }
        }

    }  // namespace stock

}  // namespace http::server