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
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

// SSL include
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "response.h"

namespace http {

    class Client {
       public:
        Client(const std::string& url);
        void SetCert(const std::string& cert_file);

        std::optional<Response> Get(const std::string& path);
        std::optional<Response> Post(const std::string& path, const std::string& body);
        std::optional<Response> Put(const std::string& path, const std::string& body);
        std::optional<Response> Delete(const std::string& path);

       private:
        std::string host_;
        int port_;
        bool is_https_ = false;
        bool keep_alive_ = true;
        // certificate
        std::string ca_cert_file_;
        bool use_custom_ca_ = true;

        const int kTIMEOUT_SECONDS = CLIENT_TIMEOUT_SECONDS;

        // SSL context for HTTPS
        SSL_CTX* ssl_ctx_;
        SSL* ssl_;

        int CreateSocket();

        std::string SendRequest(const std::string& method, const std::string& path);
        std::string ReadResponse(int sock);

        Response ParseResponse(const std::string& raw_response);
        http::Status ParseStatus(const std::string& raw_response);

        // SSL helper functions
        bool InitializeSSL();
        void CleanupSSL();
    };

}  // namespace http

#endif
