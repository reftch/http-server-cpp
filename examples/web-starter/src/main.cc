#include <signal.h>

#include <chrono>
#include <iomanip>
#include <sstream>

#include "server.h"

int main() {
    http::server s("0.0.0.0", 8080);

    // Register signal handler with capture
    static auto s_ptr = &s;
    signal(SIGINT, [](int) {
        s_ptr->stop();
    });

    s.path("GET", "/", [](const http::Request&, http::Response& res) {
        res.set_content(read_file("./assets/index.html"), http::content_type::HTML);
        // return http::response::html(read_file("./assets/index.html"));
    });

    s.path("GET", "/home", [](const http::Request&, http::Response& res) {
        res.set_content(read_file("./assets/home.html"), http::content_type::HTML);
        // return http::response::html(read_file("./assets/home.html"));
    });

    s.path("GET", "/api/v1/time", [](const http::Request&, http::Response& res) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
        res.set_content("{\"time\":\"" + ss.str() + "\"}", http::content_type::JSON);
    });

    s.start();

    return 0;
}