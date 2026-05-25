
#include "client.h"
#include "server.h"

int main() {
    static auto& log = http::Logger::getInstance();
    http::Client cli("https://0.0.0.0:8443");
    // http::Client cli("http://0.0.0.0:8080");

    // for (size_t i = 0; i < 10; ++i) {
    auto res = cli.Get("/api/v1/inc/3");
    // auto res = cli.Get("/");
    if (res && res->status() == http::Status::ok) {
        log.Info("Response status: {}", static_cast<int>(res->status()));
        log.Info("Response body: {}", res->content());
        // log.Info("Response Content-Type: {}", res->headers().at("Content-Type"));
    } else {
        log.Error("Request failed");
        // break;
    }
    // }

    return 0;
}