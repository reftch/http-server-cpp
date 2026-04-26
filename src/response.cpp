#include "response.hpp"

#include <unistd.h>

#include <string>
#include <vector>

namespace http::server {

    namespace response {
        std::string get(const status& status_type, const char* type, const std::string& content) {
            //
            std::vector<header> headers;
            headers.resize(3);
            headers[0].name = "Content-Type";
            headers[0].value = type;
            headers[1].name = "Content-Length";
            headers[1].value = std::to_string(content.size());
            headers[2].name = "Connection";
            headers[2].value = "keep-alive";

            std::string body = to_string(status_type);
            for (std::size_t i = 0; i < headers.size(); ++i) {
                header& h = headers[i];
                body.append(h.name);
                body.append(misc_strings::name_value_separator);
                body.append(h.value);
                body.append(misc_strings::crlf);
            }
            body.append(misc_strings::crlf);
            body.append(content);

            return body;
        }

        std::string to_string(const status& status_type) {
            switch (status_type) {
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
                case status::not_modified:
                    return status_strings::not_modified;
                case status::bad_request:
                    return status_strings::bad_request;
                case status::unauthorized:
                    return status_strings::unauthorized;
                case status::forbidden:
                    return status_strings::forbidden;
                case status::not_found:
                    return status_strings::not_found;
                case status::internal_server_error:
                    return status_strings::internal_server_error;
                case status::not_implemented:
                    return status_strings::not_implemented;
                case status::bad_gateway:
                    return status_strings::bad_gateway;
                case status::service_unavailable:
                    return status_strings::service_unavailable;
                default:
                    return status_strings::internal_server_error;
            }
        }

    }  // namespace response

}  // namespace http::server