#include <charconv>

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
    http::Server s("0.0.0.0", 8088);
    // http::SSLServer s("0.0.0.0", 8443, "cert.pem", "key.pem");
    http::ThreadPool pool;

    // REST endpoint
    s.setRoute<http::HttpMethod::GET>("/api/v1/users/:v", [](const http::Request& req, http::Response& res) {
        res << http::ContentType::JSON << "{\"value\":\"" << req.params().at("v") << "\"}";
    });

    // SSE handler
    s.setRoute<http::HttpMethod::GET>("/stream", [&](const http::Request&, http::Response& res) {
        auto res_ptr = std::make_shared<http::Response>(std::move(res));
        pool.enqueue([res_ptr] {
            int counter = 0;
            while (true) {
                if (!res_ptr->stream(std::format("data: {} \n\n", ++counter).c_str())) break;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    });

    // Websocket handler
    s.setRoute("/wstime", [&](http::WebSocket& ws) {
        std::string msg;
        auto result = ws >> msg;
        if (result == http::Result::Fail) {
            return;
        }

        // Store ws in a shared_ptr to keep it alive
        auto ws_ptr = std::make_shared<http::WebSocket>(std::move(ws));

        // Start the background thread
        pool.enqueue([ws_ptr] {
            while (ws_ptr->isOpen()) {
                *ws_ptr << getCurrentTimeJson();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    });

    // Post request for CORS
    s.setPostRoute([&](const http::Request&, http::Response& res) {
        res.setHeader("Access-Control-Allow-Origin", "*");
        res.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    });

    s.run();

    return 0;
}
