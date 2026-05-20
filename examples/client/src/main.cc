
#include "server.h"

int main() {
    static auto& log = http::Logger::getInstance();
    http::Client cli("http://localhost:8080");

    auto res = cli.Get("/api/v1/inc/2");
    if (res) {
        log.Info("Response status: {}, body: {}", res->status, res->body);
    } else {
        log.Error("Request failed");
    }

    return 0;
}