#ifndef HTTP_HEADER_H_
#define HTTP_HEADER_H_

#include <string>

namespace http {
    struct Header {
        std::string key;
        std::string value;
    };
}  // namespace http

#endif  // HEADER_HPP