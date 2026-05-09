#include <signal.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>

// HTTP server implementation
#include "server.hpp"

http::server* server_ptr = nullptr;

int main() {
    // setup the server object using std::make_unique
    auto s_ptr = std::make_unique<http::server>("127.0.0.1", 8080);

    // get a raw pointer to pass to the signal handler
    server_ptr = s_ptr.get();

    // handlers
    s_ptr->path("GET", "/", [](const std::string&, const auto&) {
        std::string json = R"({"name": "Alice"})";

        // std::unordered_map<std::string, std::string> mutable_params = {
            // {"name", "Alice"}, {"age", "33"}, {"id", "1"}};
        // std::string json = map_to_json(mutable_params);

        return http::response::json(json);
    });

    s_ptr->path("GET", "/api/v1/users/:id", [](const std::string&, const auto& params) {
        std::unordered_map<std::string, std::string> mutable_params = {
            {"name", "Alice"}, {"age", "33"}, {"id", params.at("id")}};

        // This now calls the defined helper function
        std::string json = map_to_json(mutable_params);
        return http::response::json(json);
    });

    s_ptr->path("GET", "/api/v1/users/:id/age/:age", [](const std::string&, const auto& params) {
        std::unordered_map<std::string, std::string> mutable_params = {
            {"name", "Alice"}, {"age", params.at("age")}, {"id", params.at("id")}};

        // This now calls the defined helper function
        std::string json = map_to_json(mutable_params);
        return http::response::json(json);
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