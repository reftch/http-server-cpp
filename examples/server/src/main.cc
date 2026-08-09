// #define HTTP_OPENSSL_SUPPORT
// #include "sslserver.h"

#include <chrono>
#include <ratio>

#include "server.h"

[[nodiscard]]
std::string getCurrentTimeJson() {
    // Get current time
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    // Convert to local time structure
    std::tm local_tm = *std::localtime(&now_c);

    // Use stringstream to format the date
    std::stringstream ss;
    ss << std::put_time(&local_tm, "%d.%m.%Y %H:%M:%S");

    return std::format(R"({{"content":"<div id='wstime'>{}</div>"}})", ss.str());
}

int main() {
    http::Server s("0.0.0.0", 8088);
    // auto& log = http::Logger::getInstance();
    http::Logger::getInstance().setLevel(http::Level::DEBUG);
    // http::SSLServer s("0.0.0.0", 8443, "cert.pem", "key.pem");

    // REST endpoint
    s.setRoute<http::HttpMethod::GET>("/api/v1/users/:v", [](const http::Request& req, http::Response& res) {
        res << http::ContentType::JSON << "{\"value\":\"" << req.params().at("v") << "\"}";
    });

    int counter = 0;

    // SSE handler
    s.setRoute<http::HttpMethod::GET>("/stream", [&](const http::Request&, http::Response& res) {
        auto res_ptr = std::make_shared<http::Response>(std::move(res));

        s.taskQueue()->enqueue("/stream", [res_ptr, &counter] {
            while (true) {
                if (!res_ptr->stream(std::format("data: {}\n\n", ++counter).c_str())) break;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    });

    // WebSocket handler
    // s.setRoute("/wstime", [&](http::WebSocket& ws) {
    //     auto ws_ptr = std::make_shared<http::WebSocket>(std::move(ws));
    //     // s.taskQueue()->remove("/wstime");

    //     s.taskQueue()->enqueue("/wstime", [ws_ptr] {
    //         while (ws_ptr->isOpen()) {
    //             ws_ptr->send(getCurrentTimeJson());
    //             std::this_thread::sleep_for(std::chrono::seconds(1));
    //         }
    //     });
    // });

    // Post request for CORS
    s.setPostRoute([&](const http::Request&, http::Response& res) {
        res.setHeader("Access-Control-Allow-Origin", "*");
        res.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    });

    s.run();

    return 0;
}
