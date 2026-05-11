#include <signal.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>

#include "server.hpp"

http::server* server_ptr = nullptr;

int main() {
    // setup the server object using std::make_unique
    auto s_ptr = std::make_unique<http::server>("127.0.0.1", 8080);

    // get a raw pointer to pass to the signal handler
    server_ptr = s_ptr.get();

    // handlers
    s_ptr->path("GET", "/", [](const std::string&, const auto&) {
        auto content = read_file("./static/index.html");
        return http::response::create(http::response::content_type::HTML, content);
    });

    s_ptr->path("GET", "/home", [](const std::string&, const auto&) {
        auto content = read_file("./static/home.html");
        return http::response::create(http::response::content_type::HTML, content);
    });

    // register the signal handler using lambda
    signal(SIGINT, [](int) {
        if (server_ptr) {
            server_ptr->stop();
        }
    });

    // start the server
    s_ptr->start();

    return 0;
}