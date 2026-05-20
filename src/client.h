#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_

#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "response.h"

namespace http {

    class Client {
       private:
        std::string host;
        int port;
        bool is_https = false;
        int timeout_seconds = 10;

        int create_socket();

        std::string send_request(const std::string& method, const std::string& path);

        Response parse_response(const std::string& raw_response);

       public:
        Client(const std::string& url);

        std::optional<Response> Get(const std::string& path);
        std::optional<Response> Post(const std::string& path, const std::string& body);
        std::optional<Response> Put(const std::string& path, const std::string& body);
        std::optional<Response> Delete(const std::string& path);
    };

}  // namespace http

#endif
