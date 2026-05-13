#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <string>
#include <unordered_map>

#include "request.hpp"
#include "response.hpp"

namespace http {
    struct context {
        http::request::http_request request;

        context(const http::request::http_request& request_) : request(request_) {}
    };
}  // namespace http

#endif