#include <chrono>

// #define HTTP_OPENSSL_SUPPORT
// #include "sslserver.h"

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
    http::Server s("0.0.0.0", 8080);
    // http::WorkerPool pool(4);
    // auto& log = http::Logger::getInstance();
    // http::Logger::getInstance().setLevel(http::Level::DEBUG);
    // http::SSLServer s("0.0.0.0", 8443, "cert.pem", "key.pem");

    // REST endpoint
    s.setRoute<http::HttpMethod::GET>("/api/v1/users/:v", [](const http::Request& req, http::Response& res) {
        res << http::ContentType::JSON << "{\"value\":\"" << req.params().at("v") << "\"}";
    });

    int counter = 0;

    // SSE handler
    s.setRoute<http::HttpMethod::GET>("/stream", [&](const http::Request&, http::Response& res) {
        s.repeatEvery(res.getId(), std::chrono::seconds(1), [res, &counter]() mutable {
            return res.stream(std::format("data: {}\n\n", ++counter).c_str());
        });
    });

    // WebSocket handler
    s.setRoute("/wstime", [&](http::WebSocket& ws) {
        s.repeatEvery(ws.getId(), std::chrono::seconds(1), [ws]() mutable {
            return ws.send(getCurrentTimeJson());
        });
    });

    // Post request for CORS
    // s.setPostRoute([&](const http::Request&, http::Response& res) {
    //     res.setHeader("Access-Control-Allow-Origin", "*");
    //     res.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    //     res.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    // });

    s.run();

    return 0;
}
