#include <signal.h>

#include <chrono>
#include <iomanip>
#include <sstream>

#include "server.hpp"

int main() {
    auto server = std::make_unique<http::server>("0.0.0.0", 8080);

    // Register signal handler with capture
    static http::server* server_ptr = server.get();
    signal(SIGINT, [](int) { server_ptr->stop(); });

    server
        ->path("GET", "/",
               [](const std::string&, const auto&, const auto&) {
                   // index.html
                   return http::response::html(read_file("./assets/index.html"));
               })
        .path("GET", "/home",
              [](const std::string&, const auto&, const auto&) {
                  // home.html
                  return http::response::html(read_file("./assets/home.html"));
              })
        .path("GET", "/api/v1/time",
              [](const std::string&, const auto&, const auto&) {
                  auto now = std::chrono::system_clock::now();
                  auto time_t = std::chrono::system_clock::to_time_t(now);
                  std::stringstream ss;
                  ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
                  return http::response::json("{\"time\":\"" + ss.str() + "\"}");
              })
        .start();

    return 0;
}