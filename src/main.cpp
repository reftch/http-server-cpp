#include "server.hpp"
#include "utils.hpp"
#include <iostream>
#include <thread>

int main(int argc, char *argv[])
{
    // --- 1. Initialize defaults ---
    std::string host = "0.0.0.0";  // Default host
    std::string port = "8080";     // Default port
    bool show_help = false;

    // --- 2. Parse Flags (Overriding Defaults) ---

    // Iterate through arguments starting from index 1
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help") {
            show_help = true;
            break;
        } else if (arg == "--host" && i + 1 < argc) {
            host = argv[++i]; // Use the provided host
        } else if (arg == "--port" && i + 1 < argc) {
            try {
                port = argv[++i]; // Use the provided port
            } catch (...) {
                std::cerr << "Error: Port must be a number.\n";
                return 1;
            }
        }
    }

    // --- 3. Handle Help Request ---
    if (show_help) {
        print_help();
        return 0;
    }

    // --- 4. Handle Positional Arguments (Address, Port) ---
    // If the user provides positional arguments, they override the flags and defaults.
    if (argc >= 3) {
        // Positional arguments are argv[1] (address) and argv[2] (port)
        host = argv[1];
        port = argv[2];
    }
    // If argc < 3, host and port retain their default values.

    // Get the build date string
    std::string build_date = get_build_date();
    
    std::cout << "HTTP Server, v1.0.0 (Build: " << build_date << ")" << std::endl;
    
    // --- 5. Server Execution ---
    try {
        // Pass the final configuration to the server
        http::server::server s(host, port);
        s.start();
    } catch (const std::exception &e)
    {
        std::cerr << "Server Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}