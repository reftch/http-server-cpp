#include "utils.hpp"

#include <ctime>    // For time functions
#include <iomanip>  // For stream manipulation (like std::put_time)
#include <iostream>
#include <map>
#include <sstream>  // Must include this for stringstream
#include <vector>

std::string get_build_date() {
    // Get current time
    std::time_t now = std::time(nullptr);

    // Convert to local time structure
    std::tm* ltm = std::localtime(&now);

    // Format the date (e.g., YYYY-MM-DD)
    std::stringstream ss;

    // Extract components: Day, Month, Year
    // Note: %d is the day, %m is the month, %Y is the year
    ss << std::put_time(ltm, "%d.%m.%Y");

    return ss.str();
}

// --- Helper function for help message ---
void print_help() {
    std::cout << "Usage: http_server [options] <address> <port>\n\n"
              << "Options (Optional):\n"
              << "  --help         Show this help message.\n"
              << "  --host [addr]  Specify the IP address (default: 0.0.0.0)\n"
              << "  --port [num]   Specify the port number (default: 8080)\n";
}

std::map<std::string, std::string> parseArguments(int argc, char* argv[]) {
    std::map<std::string, std::string> args;

    // Set default values
    args["--host"] = "0.0.0.0";
    args["--port"] = "8080";
    args["--help"] = "false";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--host" && i + 1 < argc) {
            args["--host"] = argv[i + 1];
            i++;
        } else if (arg == "--port" && i + 1 < argc) {
            args["--port"] = argv[i + 1];
            i++;
        } else if (arg == "--help") {
            args["--help"] = "true";
        }
    }

    return args;
}