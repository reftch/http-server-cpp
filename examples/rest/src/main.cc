#include "server.h"

int main() {
    http::Server s("0.0.0.0", 8082);

    s.setRoute<http::HttpMethod::GET>("/api/v1/inc/:id", [&](const http::Request& req, http::Response& res) {
        auto& params = req.params();
        auto it = params.find("id");

        if (it != params.end()) {
            std::string id = it->second;
            res << http::ContentType::JSON << "{\"value\":\"" + id + "\"}";
        }
    });

    s.run();

    return 0;
}
