#include <iostream>
#include <thread>

#include "server.hpp"
#include "utils.hpp"

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
    std::cout << "http server, v1.0.0 (Build: " << build_date << ")" << std::endl;

    // Server Execution
    try {
        // Pass the final configuration to the server
        http::server::server s(args["--host"], args["--port"]);
        s.start();
    } catch (const std::exception& e) {
        std::cerr << "Server Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}