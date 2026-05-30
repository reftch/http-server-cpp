#include "client.h"
#include "server.h"

int main() {
    static auto& log = http::Logger::getInstance();

    http::Client cli("https://api.open-meteo.com");

    // Berlin: 52.51786001257598, 13.403031762020568
    auto res = cli.Get("/v1/forecast?latitude=52.51786001257598&longitude=13.403031762020568&current=temperature_2m");

    log.Info("Response status: {}", static_cast<int>(res->status()));
    log.Info("Response body: {}", res->content());
    // log.Info("Response Content-Type: {}", res->headers().at("Content-Type"));

    // http::Client cli("https://photon.komoot.io");

    // auto res = cli.Get("/api?q=stuttgart");

    // log.Info("Response status: {}", static_cast<int>(res->status()));
    // log.Info("Response body: {}", res->content());
    // log.Info("Response Content-Type: {}", res->headers().at("Content-Type"));

    return 0;
}
