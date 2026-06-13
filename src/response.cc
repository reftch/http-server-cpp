#include "response.h"

namespace http {

    Response::Response(bool is_keep_alive, const std::string& static_directory) : static_directory_(static_directory) {
        if (is_keep_alive) {
            setHeader("Connection", "keep-alive");
        }
    }

    std::string Response::statusToString() {
        switch (status_) {
            case Status::ok:
                return statusToStrings::ok;
            case Status::created:
                return statusToStrings::created;
            case Status::accepted:
                return statusToStrings::accepted;
            case Status::no_content:
                return statusToStrings::no_content;
            case Status::multiple_choices:
                return statusToStrings::multiple_choices;
            case Status::moved_permanently:
                return statusToStrings::moved_permanently;
            case Status::moved_temporarily:
                return statusToStrings::moved_temporarily;
            case Status::not_modified:
                return statusToStrings::not_modified;
            case Status::bad_request:
                return statusToStrings::bad_request;
            case Status::unauthorized:
                return statusToStrings::unauthorized;
            case Status::forbidden:
                return statusToStrings::forbidden;
            case Status::not_found:
                return statusToStrings::not_found;
            case Status::internal_server_error:
                return statusToStrings::internal_server_error;
            case Status::not_implemented:
                return statusToStrings::not_implemented;
            case Status::bad_gateway:
                return statusToStrings::bad_gateway;
            case Status::service_unavailable:
                return statusToStrings::service_unavailable;
            default:
                return statusToStrings::internal_server_error;
        }
    }

    std::string Response::build() {
        std::string body = statusToString();

        // Add all headers (order not guaranteed, but acceptable for HTTP)
        for (const auto& header : headers_) {
            body.append(header.first);
            body.append(miscStrings::name_value_separator);
            body.append(header.second);
            body.append(miscStrings::crlf);
        }

        body.append(miscStrings::crlf);
        body.append(content_);

        return body;
    }

}  // namespace http