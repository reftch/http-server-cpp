#include <charconv>
#include <chrono>
#include <format>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "response.h"
#include "server.h"
// #include "websocket.h"

// #define HTTP_OPENSSL_SUPPORT
// #include "response.h"
// #include "sslserver.h"

using json = nlohmann::json;

[[nodiscard]]
std::string getCurrentTimeJson() {
    auto now = std::chrono::system_clock::now();

    auto local_time =
        std::chrono::zoned_time{std::chrono::current_zone(), std::chrono::floor<std::chrono::seconds>(now)};

    return std::format(R"({{"content":"<div id='wstime'>{:%d.%m.%Y %H:%M:%S}</div>"}})", local_time);
}

int main() {
    static auto& log = http::Logger::getInstance();
    log.setLevel(http::Level::DEBUG);

    // http::Server s("0.0.0.0", 8083);
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
    s.setRoute<http::HttpMethod::GET>("/stream", [](const http::Request&, http::Response& res) {
        res << http::ContentType::SSE << "data: connected\n\n";
        res.sendChunk();

        auto res_ptr = std::make_shared<http::Response>(std::move(res));
        std::thread([res_ptr]() {
            int i = 0;
            auto result = true;
            while (result) {
                *res_ptr << "data: " << (++i) << "\n\n";
                result = res_ptr->sendChunk();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }).detach();
    });

    s.setRoute("/chatroom", [](http::WebSocket& ws) {
        std::string msg;
        auto result = ws >> msg;
        if (result != http::Result::Fail) {
            auto data = json::parse(msg).at("body");
            auto message = data.at("message").get<std::string>();

            log.info("Received websocket message {}", message);
            ws << std::format(R"({{"content":"<div id='messages'>{}</div>"}})", message);
        }
    });

    // Websocket handler
    s.setRoute("/wscount", [](http::WebSocket& ws) {
        std::string msg;
        auto result = ws >> msg;
        if (result != http::Result::Fail) {
            log.info("Websocket message is {}", 1);
        }

        // Store ws in a shared_ptr to keep it alive
        auto ws_ptr = std::make_shared<http::WebSocket>(std::move(ws));

        // Start the background thread
        std::thread([ws_ptr]() {
            // int i = 0;
            while (ws_ptr->isOpen()) {
                *ws_ptr << getCurrentTimeJson();
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
