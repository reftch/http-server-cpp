#ifndef HTTP_UTILS_H_
#define HTTP_UTILS_H_

#include <sys/stat.h>

#include <algorithm>
#include <chrono>
#include <ctime>  // For time functions
#include <filesystem>
#include <fstream>
#include <iomanip>  // For stream manipulation (like std::put_time)
#include <iostream>
#include <map>
#include <optional>
#include <sstream>  // Must include this for stringstream
#include <string>
#include <unordered_map>
#include <vector>

namespace utils {

    /**
     * @brief Retrieves the current system build date in YYYY-MM-DD format.
     *
     * This function returns a string representing the current build date.
     *
     * @return A string containing the current date in YYYY-MM-DD format.
     */
    std::string getBuildDate();

    /**
     * @brief Prints the application's usage instructions and available options.
     *
     * Displays a help message to the user with information about how to use the
     * application and what command-line options are available.
     */
    void printHelp();

    /**
     * @brief Parses command-line arguments for host and port configuration.
     *
     * Processes command-line arguments and returns a map containing the parsed
     * host and port values. If no arguments are provided, default values are used:
     * - Host: "0.0.0.0"
     * - Port: "8080"
     *
     * @param argc The number of command-line arguments.
     * @param argv An array of command-line argument strings.
     * @return A map containing the parsed arguments with keys "--host" and "--port".
     *
     * @example
     * parseArguments(3, {"server", "--host", "127.0.0.1"})
     * // Returns map with "--host": "127.0.0.1" and "--port": "8080"
     *
     * @example
     * parseArguments(5, {"server", "--port", "9000", "--host", "192.168.1.1"})
     * // Returns map with "--host": "192.168.1.1" and "--port": "9000"
     */
    std::map<std::string, std::string> parseArguments(int argc, char* argv[]);

    /**
     * @brief Converts an unordered map to a JSON string.
     *
     * This function takes a map of string key-value pairs and converts it into
     * a JSON-formatted string representation.
     *
     * @param params The unordered map to convert to JSON.
     * @return A string containing the JSON representation of the input map.
     */
    std::string mapToJson(const std::unordered_map<std::string, std::string>& params);

    /**
     * @brief Reads the contents of a file into a string.
     *
     * Opens a file at the specified path and reads its entire content into a string.
     *
     * @param path The file path to read from.
     * @return A string containing the file's contents.
     */
    std::string readFile(const std::string& path);

    /**
     * @brief Decodes a URL-encoded string.
     *
     * This function performs URL decoding on an encoded string, converting
     * percent-encoded characters back to their original form.
     *
     * @param encoded The URL-encoded string to decode.
     * @return A decoded string.
     */
    std::string urlDecode(const std::string& encoded);

    /**
     * @brief Retrieves an environment variable value with a default fallback.
     *
     * This template function fetches the value of an environment variable by key,
     * converting it to the specified type. If the environment variable is not set
     * or conversion fails, it returns the provided default value.
     *
     * @tparam T The type to convert the environment variable value to.
     * @param key The name of the environment variable to retrieve.
     * @param default_value The fallback value if the environment variable is not set or conversion fails.
     * @return The converted environment variable value or the default value.
     *
     * @example
     * auto host = GetEnv<std::string>("HOST", "0.0.0.0");
     * auto port = GetEnv<int>("PORT", 8080);
     * auto debug = GetEnv<bool>("DEBUG", false);
     */
    template <typename T>
    T getEnv(const std::string& key, const T& default_value) {
        const char* value = std::getenv(key.c_str());

        if (!value) {
            return default_value;
        }

        try {
            if constexpr (std::is_same_v<T, std::string>) {
                return std::string(value);
            } else if constexpr (std::is_same_v<T, bool>) {
                std::string lower_value = std::string(value);
                std::transform(lower_value.begin(), lower_value.end(), lower_value.begin(), ::tolower);
                if (lower_value == "true" || lower_value == "1" || lower_value == "yes") {
                    return true;
                } else if (lower_value == "false" || lower_value == "0" || lower_value == "no") {
                    return false;
                } else {
                    throw std::invalid_argument("Invalid boolean value");
                }
            } else if constexpr (std::is_same_v<T, float>) {
                return std::stof(value);
            } else if constexpr (std::is_same_v<T, double>) {
                return std::stod(value);
            } else if constexpr (std::is_same_v<T, long double>) {
                return std::stold(value);
            } else {
                // For integer types
                return static_cast<T>(std::stoll(value));
            }
        } catch (const std::exception&) {
            return default_value;
        }
    }

    time_t getMtime(const std::string& path);

    std::string fileMtimeToHttpDate(time_t mtime);

    std::string computeEtag(size_t mtime_raw, size_t size);

    std::string fromIntToHex(size_t n);

    std::string sha1(const std::string& input);
    std::string base64_encode(const std::string& input);

    bool isWebSocketFrame(const std::string& data);

}  // namespace utils
#endif  // HTTP_UTILS_H_