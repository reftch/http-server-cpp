#include <charconv>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "response.h"
#include "server.h"
#include "websocket.h"

// #define HTTP_OPENSSL_SUPPORT
// #include "response.h"
// #include "sslserver.h"

[[nodiscard]]
std::string getCurrentTimeJson() {
    auto now = std::chrono::system_clock::now();
    return std::format(R"({{"time":"{:%Y-%m-%dT%H:%M:%SZ}"}})", now);
}

int main() {
    static auto& log = http::Logger::getInstance();
    // log.setLevel(http::Level::DEBUG);

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
        auto it = req.params().find("v");
        if (it == req.params().end()) {
            res << http::ContentType::JSON << http::Status::bad_request << R"({"error":"missing parameter 'v'"})";
            return;
        }

        int val = 0;
        if (!std::from_chars(it->second.data(), it->second.data() + it->second.size(), val).ptr) {
            res << http::ContentType::JSON << http::Status::bad_request << R"({"error":"invalid integer"})";
            return;
        }

        res << http::ContentType::JSON << "{\"value\":\"" + std::to_string(val + 1) + "\"}";
    });

    // SSE handler
    s.setRoute<http::HttpMethod::GET>("/events", [](const http::Request&, http::Response& res) {
        res << http::ContentType::SSE << "data: connected\n\n";
        res.sendChunk();

        auto res_ptr = std::make_shared<http::Response>(std::move(res));

        std::thread([res_ptr]() {
            // std::jthread worker([res_ptr]() {
            auto result = true;
            while (result) {
                *res_ptr << "data: " << getCurrentTimeJson() << "\n\n";
                result = res_ptr->sendChunk();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }).detach();
    });

    // Websocket handler
    s.setRoute("/ws", [](http::WebSocket& ws) {
        std::string msg;
        auto result = ws >> msg;
        if (result != http::Result::Fail) {
            log.info("Received websocket message {}", msg);
        }

        // Store ws in a shared_ptr to keep it alive
        auto ws_ptr = std::make_shared<http::WebSocket>(std::move(ws));

        // Start the background thread
        std::thread([ws_ptr]() {
            while (ws_ptr->isOpen()) {
                *ws_ptr << getCurrentTimeJson();
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
