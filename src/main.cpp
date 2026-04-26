#include <iostream>
#include <thread>

#include "server.hpp"
#include "utils.hpp"

// Handler for the root path (GET /)
std::string home_handler(const std::string& path, const std::unordered_map<std::string, std::string>& params) {
    std::string json_string = R"({"name": "Alice"})";
    return json_string;
}

std::string api_handler(const std::string& path, const std::unordered_map<std::string, std::string>& params) {
    std::unordered_map<std::string, std::string> mutable_params = {
        {"name", "Alice"}, {"age", "34"}, {"id", params.at("id")}};
    std::string json_output = map_to_json(mutable_params);
    return json_output;
}

int main(int argc, char* argv[]) {
    // parse input parameters
    auto args = parseArguments(argc, argv);

    // show help message
    if (args["--help"] == "true") {
        print_help();
        return 0;
    }

    // Get the build date string
    std::string build_date = get_build_date();
    // Launch banner
    std::cout << "http server, v1.0.0 (Build: " << build_date << ")" << '\n';

    // Server Execution
    try {
        // Pass the final configuration to the server
        http::server::server s(args["--host"], args["--port"]);
        s.register_handler("GET", "/", home_handler);
        s.register_handler("GET", "/api/v1/users/:id", api_handler);
        s.start();
    } catch (const std::exception& e) {
        std::cerr << "Server Exception: " << e.what() << '\n';
        return 1;
    }

    return 0;
}