#ifndef HEADER_HPP
#define HEADER_HPP

#include <string>

namespace http::server {
    struct header {
        std::string name;
        std::string value;
    };
}  // namespace http::server

#endif  // HEADER_HPP