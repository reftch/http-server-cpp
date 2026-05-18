#include <signal.h>

#include <chrono>
#include <iomanip>
#include <sstream>

#include "server.h"

int main() {
    http::Server s("0.0.0.0", 8080);

    // Register signal handler with capture
    static auto s_ptr = &s;
    signal(SIGINT, [](int) {
        s_ptr->Stop();
    });

    s.SetRoute<http::HttpMethod::GET>("/", [](const http::Request&, http::Response& res) {
        res.SetContent<http::ContentType::HTML>(ReadFile("./assets/index.html"));
    });

    s.SetRoute<http::HttpMethod::GET>("/home", [](const http::Request&, http::Response& res) {
        res.SetContent<http::ContentType::HTML>(ReadFile("./assets/home.html"));
    });

    s.SetRoute<http::HttpMethod::GET>("/api/v1/time", [](const http::Request&, http::Response& res) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
        res.SetContent<http::ContentType::JSON>("{\"time\":\"" + ss.str() + "\"}");
    });

    s.SetRoute<http::HttpMethod::GET>("/api/v1/users/:id/:age", [](const http::Request& req, http::Response& res) {
        auto id = req.params().at("id");
        auto age = req.params().at("age");
        res.SetContent<http::ContentType::JSON>("{\"id\":\"" + id + ",\" + \"age\":\"" + age + "\"}");
    });

    s.Start();

    return 0;
}