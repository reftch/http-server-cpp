#include <signal.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>

// HTTP server implementation
#include "server.hpp"

// std::string home_handler(const std::string& path, const std::unordered_map<std::string, std::string>& params) {
//     std::string json = R"({"name": "Alice"})";
//     return http::response::json(json);
// }

// std::string api_handler(const std::string& path, const std::unordered_map<std::string, std::string>& params) {
//     // Note: Using .at() assumes "id" is always present in params.
//     std::unordered_map<std::string, std::string> mutable_params = {
//         {"name", "Alice"}, {"age", "34"}, {"id", params.at("id")}};

//     // This now calls the defined helper function
//     std::string json = map_to_json(mutable_params);
//     return http::response::json(json);
// }

http::server* server_ptr = nullptr;

int main() {
    // setup the server object using std::make_unique
    auto s_ptr = std::make_unique<http::server>("127.0.0.1", 8080);

    // get a raw pointer to pass to the signal handler
    server_ptr = s_ptr.get();

    // register handlers

    s_ptr->g_router.register_handler("GET", "/", [](const std::string&, const auto&) {
        std::string json = R"({"name": "Alice"})";
        return http::response::json(json);
    });

    s_ptr->g_router.register_handler("GET", "/api/v1/users/:id", [](const std::string&, const auto& params) {
        // const std::string& id = params.at("id");
        // return http::response::json("{\"id\":\"" + id + "\",\"name\":\"user\"}");

        // Note:
        // Using.at() assumes "id" is always present in params.
        std::unordered_map<std::string, std::string> mutable_params = {
            {"name", "Alice"}, {"age", "34"}, {"id", params.at("id")}};

        // This now calls the defined helper function
        std::string json = map_to_json(mutable_params);
        return http::response::json(json);
    });

    // s_ptr->register_handler("GET", "/", home_handler);
    // s_ptr->register_handler("GET", "/api/v1/users/:id", api_handler);

    // register the signal handler using lambda
    signal(SIGINT, [](int sig) {
        if (server_ptr) {
            server_ptr->stop();
        }
    });

    // start the server
    s_ptr->start();

    return 0;
}