#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <vector>

#include "header.hpp"

namespace http::server {

    enum status_type {
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
    } status;

    struct content_type {
        std::string mime_type;  // The standard MIME type (e.g., "application/json")
        std::string category;   // A descriptive category (e.g., "data", "media", "text")
    };

    const content_type json_type = {"application/json", "data"};
    const content_type image_type = {"image/jpeg", "media"};
    const content_type text_type = {"text/plain", "text"};

    class response {
       public:
        // std::vector<header> getHeaders();

        response(const status_type& status, const content_type& type, const std::string& content);
        response(const status_type& status, const content_type& type, const std::vector<header> headers,
                 const std::string& content);

        std::string get_body();

       private:
        status_type status_;
        // The headers to be included in the reply.
        std::vector<header> headers_;
        // The content to be sent in the reply.
        std::string content_;

        std::string to_string();
    };

}  // namespace http::server
#endif  // RESPONSE_HPP