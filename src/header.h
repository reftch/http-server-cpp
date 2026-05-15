#ifndef HEADER_HPP
#define HEADER_HPP

#include <string>

namespace http {
    struct header {
        std::string name;
        std::string value;
    };
}  // namespace http

#endif  // HEADER_HPP