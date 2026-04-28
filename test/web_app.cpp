#include <signal.h>
#include <unistd.h>  // For exit()

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>

// assuming this is the header for your HTTP server implementation
#include "server.hpp"

// must define the function used in api_handler
std::string map_to_json(const std::unordered_map<std::string, std::string>& params) {
    // Example implementation
    std::string json = "{";
    bool first = true;
    for (const auto& pair : params) {
        if (!first) {
            json += ", ";
        }
        json += "\"" + pair.first + "\": \"" + pair.second + "\"";
        first = false;
    }
    json += "}";
    return json;
}

std::string home_handler(const std::string& path, const std::unordered_map<std::string, std::string>& params) {
    std::string json_string = R"({"name": "Alice"})";
    return json_string;
}

std::string api_handler(const std::string& path, const std::unordered_map<std::string, std::string>& params) {
    // Note: Using .at() assumes "id" is always present in params.
    std::unordered_map<std::string, std::string> mutable_params = {
        {"name", "Alice"}, {"age", "34"}, {"id", params.at("id")}};

    // This now calls the defined helper function
    std::string json_output = map_to_json(mutable_params);
    return json_output;
}

http::server::server* server_ptr;

// Signal Handler Function
// This function is executed when SIGINT (Ctrl+C) is received.
void signal_handler(int sig) {
    // Set the running flag to false on the server object
    if (server_ptr) {
        server_ptr->stop();
    }
}

int main() {
    // setup the server object
    http::server::server s("127.0.0.1", "8080");
    s.register_handler("GET", "/", home_handler);
    s.register_handler("GET", "/api/v1/users/:id", api_handler);

    // --- Register the Signal Handler ---
    server_ptr = &s;
    // Register the signal_handler function to intercept SIGINT
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("Failed to register signal handler");
        return 1;
    }

    s.start();

    return 0;
}