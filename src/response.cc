#include "response.h"

#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

namespace http {

    Response::Response(bool is_keep_alive) {
        if (is_keep_alive) {
            set_header("Connection", "keep-alive");
        }
    }

    void Response::set_content(const Status& status, const std::string& content, const std::string& content_type) {
        set_header("Content-Type", content_type);
        set_header("Content-Length", std::to_string(content.size()));
        content_ = content;
        status_ = status;
    }

    void Response::set_content(const std::string& content, const std::string& content_type) {
        set_content(Status::ok, content, content_type);
    }

    void Response::set_html(const std::string& content) { set_content(Status::ok, content, content_type::HTML); }

    void Response::set_json(const std::string& content) { set_content(Status::ok, content, content_type::JSON); }

    std::string Response::status() {
        switch (status_) {
            case Status::ok:
                return status_strings::ok;
            case Status::created:
                return status_strings::created;
            case Status::accepted:
                return status_strings::accepted;
            case Status::no_content:
                return status_strings::no_content;
            case Status::multiple_choices:
                return status_strings::multiple_choices;
            case Status::moved_permanently:
                return status_strings::moved_permanently;
            case Status::moved_temporarily:
                return status_strings::moved_temporarily;
            case Status::not_modified:
                return status_strings::not_modified;
            case Status::bad_request:
                return status_strings::bad_request;
            case Status::unauthorized:
                return status_strings::unauthorized;
            case Status::forbidden:
                return status_strings::forbidden;
            case Status::not_found:
                return status_strings::not_found;
            case Status::internal_server_error:
                return status_strings::internal_server_error;
            case Status::not_implemented:
                return status_strings::not_implemented;
            case Status::bad_gateway:
                return status_strings::bad_gateway;
            case Status::service_unavailable:
                return status_strings::service_unavailable;
            default:
                return status_strings::internal_server_error;
        }
    }

    std::string Response::build() {
        std::string body = status();

        // Add all headers (order not guaranteed, but acceptable for HTTP)
        for (const auto& header : headers_) {
            body.append(header.first);
            body.append(misc_strings::name_value_separator);
            body.append(header.second);
            body.append(misc_strings::crlf);
        }

        body.append(misc_strings::crlf);
        body.append(content_);

        return body;
    }

}  // namespace http