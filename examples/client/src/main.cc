#define HTTP_OPENSSL_SUPPORT

#include "client.h"

int main() {
    // auto http_client = http::HttpClientFactory::createClient("http://0.0.0.0:8080");
    // auto response = http_client->get("/api/v1/inc/100");

    // if (response) {
    //     // std::cout << "HTTP Status: " << response->status() << std::endl;
    //     std::cout << "Response Body: " << response->content() << std::endl;
    // } else {
    //     std::cerr << "HTTP Error: " << response.error() << std::endl;
    // }

    static auto& log = http::Logger::getInstance();

    // http::Client cli("http://0.0.0.0:8080");
    http::Client cli("https://0.0.0.0:8443");
    //

    auto res = cli.get("/api/v1/inc/2");

    log.info("Response status: {}", static_cast<int>(res->status()));
    log.info("Response body: {}", res->content());
    log.info("Response Content-Type: {}", res->headers().at("Content-Type"));

    return 0;
}
