#include <unistd.h>  // For exit()

#include <chrono>
#include <csignal>
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

// Global pointer/reference to the server object for signal handling
http::server::server* global_server_ptr = nullptr;

void signal_handler(int signum) {
    if (global_server_ptr) {
        global_server_ptr->stop();
    }

    exit(signum);
}

int main() {
    // setup the server object
    http::server::server s("127.0.0.1", "8080");
    s.register_handler("GET", "/", home_handler);
    s.register_handler("GET", "/api/v1/users/:id", api_handler);

    // set the global pointer so the signal handler can access the server
    global_server_ptr = &s;

    // setup signal handling for SIGINT (Ctrl+C)
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        perror("sigaction");
        return 1;
    }

    // start the server in a separate thread
    std::thread server_thread([&s]() { s.start(); });

    std::cout << "Server thread started. Press Ctrl+C to stop." << std::endl;

    // wait for the thread to finish
    server_thread.join();

    return 0;
}