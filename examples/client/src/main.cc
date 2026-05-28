#include "client.h"
#include "server.h"

int main() {
    static auto& log = http::Logger::getInstance();
    http::Client cli("https://api.open-meteo.com");

    auto res = cli.Get("/v1/forecast?latitude=52.52&longitude=13.41&hourly=temperature_2m");
    // auto res = cli.Get("v1/forecast?latitude=52.52&longitude=13.41&hourly=temperature_2m");
    log.Info("Response status: {}", static_cast<int>(res->status()));
    log.Info("Response body: {}", res->content());
    log.Info("Response Content-Type: {}", res->headers().at("Content-Type"));

    return 0;
}