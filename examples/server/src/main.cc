// #define HTTP_OPENSSL_SUPPORT
#include <signal.h>

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

#include "server.h"

std::string getCurrentTimeJson() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    // Format as ISO 8601 string
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%S");

    // Add milliseconds
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    ss << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";

    return "{\"time\":\"" + ss.str() + "\"}";
}

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

    s.setRoute<http::WsProtocol::WS>("/ws", [](const http::Request&, http::WebSocket& ws) {
        std::string msg;
        auto result = ws >> msg;
        if (result != http::Result::Fail) {
            log.info("Received websocket message {}", msg);
        }

        // Create a shared pointer to manage the thread lifecycle
        auto thread_ptr = std::make_shared<std::thread>([&ws]() {
            while (true) {
                // Add 1 second delay
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::string time_json = getCurrentTimeJson();
                ssize_t result = ws << time_json;
                if (result < 0) break;
            }
        });

        // Detach the thread to let it run independently
        thread_ptr->detach();
    });

    s.start();

    return 0;
}
