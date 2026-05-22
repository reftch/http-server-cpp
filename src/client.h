#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include "logger.h"
#include "response.h"

namespace http {

    class HttpResponse {
       public:
        int status = 0;
        std::string body;
        std::string status_text;
        std::string version;
        std::string headers;

        HttpResponse() = default;
        HttpResponse(int status_code, const std::string& body_text) : status(status_code), body(body_text) {}
    };

    class Client {
       private:
        std::string host;
        int port;
        bool is_https = false;
        bool keep_alive_ = true;
        int timeout_seconds = 10;

        // logger
        Logger& log = Logger::getInstance();

        int create_socket();

        std::string send_request(const std::string& method, const std::string& path);

        Response parse_response(const std::string& raw_response);
        http::Status parse_status(const std::string& raw_response);

       public:
        Client(const std::string& url);
        // Client(const std::string& url, bool keep_alive = true);

        std::optional<Response> Get(const std::string& path);
        std::optional<Response> Post(const std::string& path, const std::string& body);
        std::optional<Response> Put(const std::string& path, const std::string& body);
        std::optional<Response> Delete(const std::string& path);
    };

}  // namespace http

#endif
