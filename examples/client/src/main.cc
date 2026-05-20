
#include "server.h"

int main() {
    static auto& log = http::Logger::getInstance();
    http::Client cli("http://localhost:8080");

    auto res = cli.Get("/api/v1/inc/2");
    if (res && res->status() == http::Status::ok) {
        log.Info("Response body: {}", res->content());
    } else {
        log.Error("Request failed");
    }

    return 0;
}