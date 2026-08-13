#include <chrono>

#include "server.h"
#include "worker.h"

using namespace std::chrono_literals;

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
};

int main() {
    // http::SSLServer s("0.0.0.0", 8080, "cert.pem", "key.pem");
    http::Server s("127.0.0.1", 8082);
    auto& taskManager = http::TaskManager::get_instance();

    // REST endpoint
    s.setRoute<http::HttpMethod::GET>("/api/v1/users/:v", [](const http::Request& req, http::Response& res) {
        res << http::ContentType::JSON << "{\"value\":\"" << req.params().at("v") << "\"}";
    });

    int counter = 0;

    // SSE handler
    s.setRoute<http::HttpMethod::GET>("/stream", [&](const http::Request& req, http::Response& res) {
        taskManager.repeatEvery(req.path(), 50ms, [res, &counter]() mutable {
            return res.stream(std::format("data: {}\n\n", ++counter).c_str());
        });
    });

    // WebSocket handler
    s.setRoute("/wstime", [&](http::WebSocket& ws) {
        taskManager.repeatEvery("/wstime", 1s, [ws]() mutable {
            // s.repeatEvery(ws.getId(), 1s, [ws]() mutable {
            return ws.send(getCurrentTimeJson());
        });
    });

    // s.setRoute("/wstime", [&](http::WebSocket& ws) {
    //     s.repeatEvery(ws.getId(), 1s, [ws]() mutable {
    //         return ws.send(getCurrentTimeJson());
    //     });
    // });

    s.run();

    return 0;
}
