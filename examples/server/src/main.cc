// #define HTTP_OPENSSL_SUPPORT
#include <signal.h>

#include <chrono>
#include <iomanip>
#include <sstream>

#include "client.h"
#include "server.h"
// #include "sslserver.h"

int main() {
    static auto& log = http::Logger::getInstance();
    http::Server s("0.0.0.0", 8080);
    // http::SSLServer s("localhost", 8443, "cert.pem", "key.pem");

    // Register signal handler with capture
    static auto s_ptr = &s;
    signal(SIGINT, [](int) {
        s_ptr->stop();
    });

    s.setRoute<http::HttpMethod::GET>("/", [](const http::Request&, http::Response& res) {
        res.setContent<http::ContentType::HTML>("index.html");
    });

    s.setRoute<http::HttpMethod::GET>("/home", [](const http::Request& req, http::Response& res) {
        log.info("Request path: {}", req.path());
        res.setContent<http::ContentType::HTML>("home.html");
    });

    s.setRoute<http::HttpMethod::GET>("/api/v1/inc/:v", [](const http::Request& req, http::Response& res) {
        std::string value = req.params().at("v");
        res.setContent<http::ContentType::JSON>("{\"value\":\"" + std::to_string(std::stoi(value) + 1) + "\"}");
    });

    s.setRoute<http::HttpMethod::POST>("/api/v1/users/:id", [](const http::Request& req, http::Response&) {
        std::string value = req.params().at("id");
        log.info("Request body: {}", req.body());
    });

    s.start();

    return 0;
}
