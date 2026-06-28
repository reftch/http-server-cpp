#define HTTP_OPENSSL_SUPPORT
#include "client.h"

int main() {
    static auto& log = http::Logger::getInstance();

    // http::Client cli("http://0.0.0.0:8080");
    http::Client cli("https://0.0.0.0:8443");

    auto res = cli.get("/api/v1/inc/2");

    log.info("Response status: {}", static_cast<int>(res->status()));
    log.info("Response body: {}", res->content());
    log.info("Response Content-Type: {}", res->headers().at("Content-Type"));

    return 0;
}
