#include <signal.h>

#include <chrono>
#include <iomanip>
#include <sstream>

#include "server.h"

int main() {
    static auto& log = http::Logger::getInstance();
    http::Server s("0.0.0.0", 8080);
    log.EnableFileLogging();

    // Register signal handler with capture
    static auto s_ptr = &s;
    signal(SIGINT, [](int) {
        s_ptr->Stop();
    });

    s.SetRoute<http::HttpMethod::GET>("/", [](const http::Request&, http::Response& res) {
        res.SetContent<http::ContentType::HTML>("index.html");
    });

    s.SetRoute<http::HttpMethod::GET>("/home", [](const http::Request& req, http::Response& res) {
        log.Info("Request path: {}", req.path());
        res.SetContent<http::ContentType::HTML>("home.html");
    });

    s.SetRoute<http::HttpMethod::GET>("/api/v1/inc/:v", [](const http::Request& req, http::Response& res) {
        std::string value = req.params().at("v");
        res.SetContent<http::ContentType::JSON>("{\"value\":\"" + std::to_string(std::stoi(value) + 1) + "\"}");
    });

    s.Start();

    return 0;
}