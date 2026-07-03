#include <signal.h>

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>

#include "server.h"
// #define HTTP_OPENSSL_SUPPORT
// #include "sslserver.h"

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
    log.setLevel(http::Level::DEBUG);

    // http::Server s("0.0.0.0", 8080);
    // http::SSLServer s("localhost", 8443, "cert.pem", "key.pem");
    http::Server s;

    s.setDefaultHeaders({
        {"Connection", "keep-alive"},
    });

    s.setRoute<http::HttpMethod::GET>("/", [](const http::Request&, http::Response& res) {
        res << http::ContentType::HTML << "index.html";
    });

    s.setRoute<http::HttpMethod::GET>("/home", [](const http::Request& req, http::Response& res) {
        log.info("Request path: {}", req.path());
        res << http::ContentType::HTML << "home.html";
    });

    s.setRoute<http::HttpMethod::GET>("/api/v1/inc/:v", [&](const http::Request& req, http::Response& res) {
        std::string value = req.params().at("v");
        res << http::ContentType::JSON << "{\"value\":\"" + std::to_string(std::stoi(value) + 1) + "\"}";
    });

    s.setRoute("/ws", [](http::WebSocket& ws) {
        log.info("Token: {}", ws.query().at("token"));

        std::string msg;
        auto result = ws >> msg;
        if (result != http::Result::Fail) {
            log.info("Received websocket message {}", msg);
        }

        // Store ws in a shared_ptr to keep it alive
        auto ws_ptr = std::make_shared<http::WebSocket>(std::move(ws));

        // Start the background thread
        std::thread([ws_ptr]() {
            while (true) {
                auto time_json = getCurrentTimeJson();
                // Send message through websocket
                auto result = *ws_ptr << time_json;
                if (result < 0) {
                    log.error("WebSocket send failed");
                    break;
                }

                // Sleep for 1 seconds
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }).detach();
    });

    // Post request for CORS
    s.setPostRoute([&](const http::Request&, http::Response& res) {
        res.setHeader("Access-Control-Allow-Origin", "*");
        res.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    });

    s.start();

    return 0;
}
