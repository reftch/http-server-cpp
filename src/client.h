#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_

#include <string>

namespace http {

    class Client {
       public:
        Client(const std::string& host, const int& port) : port_(port), host_(host) {}

       private:
        const int port_;          // Port number to listen on
        const std::string host_;  // Hostname or IP address to bind to
    };
}  // namespace http

#endif
