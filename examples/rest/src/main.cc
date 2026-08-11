// #define HTTP_OPENSSL_SUPPORT
// #include "sslserver.h"

#include <csignal>

#include "server.h"

static http::Server* server = nullptr;

int main() {
    // http::SSLServer s("0.0.0.0", 8080, "cert.pem", "key.pem");
    http::Server s;

    server = &s;

    std::signal(SIGINT, [](int) {
        // std::cout << "Shutting down..." << std::endl;
        if (server) {
            server->stop();
            std::exit(0);
        }
    });

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
