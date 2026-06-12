#include "utils.h"

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

// Helper function for help message
void print_help() {
    std::cout << "Usage: http_server [options] <address> <port>\n\n"
              << "Options (Optional):\n"
              << "  --help         Show this help message.\n"
              << "  --host [addr]  Specify the IP address (default: 0.0.0.0)\n"
              << "  --port [num]   Specify the port number (default: 8080)\n";
}

std::map<std::string, std::string> ParseArguments(int argc, char* argv[]) {
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

std::string MapToJson(const std::unordered_map<std::string, std::string>& params) {
    std::stringstream ss;
    ss << "{";  // Start the JSON object

    bool first = true;

    // Iterate over all key-value pairs in the map
    for (const auto& pair : params) {
        // Add a comma separator if it's not the first element
        if (!first) {
            ss << ", ";
        }

        // Start the key (must be a quoted string)
        ss << "\"" << pair.first << "\": ";

        // Add the value (must be a quoted string)
        // Note: Since the input is string-based, we wrap the value in quotes.
        ss << "\"" << pair.second << "\"";

        first = false;
    }

    ss << "}";  // End the JSON object

    return ss.str();
}

std::string ReadFile(const std::string& path) {
    // Binary mode for faster reading
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return {};
    }

    // Get file size and read efficiently
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string content(size, '\0');
    file.read(content.data(), size);
    return content;
}

std::string UrlDecode(const std::string& encoded) {
    std::string decoded;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            // Convert hex to char
            char hex[3] = {encoded[i + 1], encoded[i + 2], '\0'};
            char c = static_cast<char>(strtol(hex, nullptr, 16));
            decoded += c;
            i += 2;
        } else if (encoded[i] == '+') {
            decoded += ' ';
        } else {
            decoded += encoded[i];
        }
    }
    return decoded;
}

time_t GetMtime(const std::string& path) {
    // Check if file exists
    if (!std::filesystem::exists(path)) {
        return -1;
    }

    // Get the file status
    auto status = std::filesystem::status(path);

    // Check if it's a regular file
    if (!std::filesystem::is_regular_file(status)) {
        return -1;
    }

    // Get the last modification time
    auto mtime = std::filesystem::last_write_time(path);

    // Convert to time_t using std::chrono::duration_cast
    auto time_t_duration = std::chrono::duration_cast<std::chrono::seconds>(mtime.time_since_epoch());

    return static_cast<time_t>(time_t_duration.count());
}

std::string FileMtimeToHttpDate(time_t mtime) {
    if (mtime < 0) {
        return std::string();
    }

    struct tm tm_buf;
    if (gmtime_r(&mtime, &tm_buf) == nullptr) {
        return std::string();
    }
    char buf[64];
    if (strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm_buf) == 0) {
        return std::string();
    }

    return std::string(buf);
}