// #define HTTP_OPENSSL_SUPPORT
// #include "sslserver.h"

#include "response.h"
#include "server.h"

int main() {
    // http::SSLServer s("0.0.0.0", 8080, "cert.pem", "key.pem");
    http::Server s;

    s.setRoute<http::HttpMethod::GET>("/api/v1/users/:id", [&](const http::Request& req, http::Response& res) {
        auto& params = req.params();
        auto it = params.find("id");

        if (it != params.end()) {
            std::string id = it->second;
            res << http::ContentType::JSON << "{\"value\":\"" + id + "\"}";
        }
    });

    s.run();

    return 0;
}
