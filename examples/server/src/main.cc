#include <signal.h>

#include <chrono>
#include <iomanip>
#include <sstream>

#include "client.h"
#include "sslserver.h"

int main() {
    static auto& log = http::Logger::getInstance();
    http::Server s("0.0.0.0", 8080);
    // http::SSLServer s("localhost", 8443, "cert.pem", "key.pem");

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

    s.SetRoute<http::HttpMethod::GET>("/api/v1/temperature", [](const http::Request&, http::Response& res) {
        http::Client cli("https://api.open-meteo.com");
        auto client_res =
            cli.Get("/v1/forecast?latitude=52.51786001257598&longitude=13.403031762020568&current=temperature_2m");
        res.SetContent<http::ContentType::JSON>(client_res->content());
    });

    s.SetRoute<http::HttpMethod::POST>("/api/v1/users/:id", [](const http::Request& req, http::Response&) {
        std::string value = req.params().at("id");
        log.Info("Request body: {}", req.body());
    });

    s.Start();

    return 0;
}
