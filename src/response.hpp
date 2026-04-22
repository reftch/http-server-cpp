#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <vector>

#include "header.hpp"

namespace http::server {
    //[response Response object to be return to a client
    struct response {
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

        // The headers to be included in the reply.
        std::vector<header> headers;

        // The content to be sent in the reply.
        std::string content;
    };
    //]

    namespace stock {
        response ok(std::string message);
        std::string to_string(response resp);
        std::string to_string(response::status_type status);
    }  // namespace stock

}  // namespace http::server

#endif  // RESPONSE_HPP