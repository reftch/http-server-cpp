#include "response.h"

#include "utils.h"

namespace http {

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

    const std::map<std::string, std::string>& Response::headers() const { return headers_; }

    std::string Response::build() {
        std::string body = statusToString();

        if (content_type_ == ContentType::HTML && content_.ends_with(".html")) {
            content_ = utils::readFile(static_directory_ + '/' + content_);
        }

        const auto& headers_ref = headers();
        auto content_type_it = headers_ref.find("Content-Type");
        if (content_type_it == headers_ref.end()) {
            setHeader("Content-Type", ContentTypeToString(content_type_));
        }

        auto content_length_it = headers_ref.find("Content-Length");
        if (content_length_it == headers_ref.end()) {
            setHeader("Content-Length", std::to_string(content_.size()));
        }

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