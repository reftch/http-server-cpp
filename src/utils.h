#ifndef HTTP_UTILS_H_
#define HTTP_UTILS_H_

#include <sys/socket.h>
#include <sys/stat.h>

#include <algorithm>
#include <cstdlib>
#include <ctime>  // For time functions
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>

namespace utils {

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

        std::string_view sv(value);

        if constexpr (std::is_same_v<T, std::string>) {
            return std::string(sv);
        } else if constexpr (std::is_same_v<T, bool>) {
            // Handle empty string case first
            if (sv.empty()) {
                return default_value;
            }

            std::string lower(sv);

            // Convert to lowercase
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

            // Check exact matches
            bool result = lower == "true" || lower == "1" || lower == "yes" || lower == "on";

            // If it matched any pattern, return the parsed value
            // If it didn't match, we should return default_value
            if (result || lower == "false" || lower == "0" || lower == "no" || lower == "off") {
                return result;
            } else {
                // Invalid boolean value - return default
                return default_value;
            }
        } else if constexpr (std::is_integral_v<T>) {
            T result{};

            if (sv.empty()) {
                return default_value;
            }

            bool negative = false;
            size_t pos = 0;

            if constexpr (std::is_signed_v<T>) {
                if (sv[0] == '-') {
                    negative = true;
                    pos = 1;
                }
            }

            for (; pos < sv.size(); ++pos) {
                const char c = sv[pos];

                if (c < '0' || c > '9') {
                    return default_value;
                }

                result = static_cast<T>(result * 10 + (c - '0'));
            }

            if constexpr (std::is_signed_v<T>) {
                if (negative) {
                    result = -result;
                }
            }

            return result;
        } else if constexpr (std::is_floating_point_v<T>) {
            char* end = nullptr;

            T result{};

            if constexpr (std::is_same_v<T, float>) {
                result = std::strtof(value, &end);
            } else if constexpr (std::is_same_v<T, double>) {
                result = std::strtod(value, &end);
            } else {
                result = static_cast<T>(std::strtold(value, &end));
            }

            if (end != value + sv.size()) {
                return default_value;
            }

            return result;
        } else {
            return default_value;
        }
    }

    time_t getMtime(const std::string& path);

    std::string fileMtimeToHttpDate(time_t mtime);

    std::string computeEtag(size_t mtime_raw, size_t size);

    std::string fromIntToHex(size_t n);

    std::string sha1(const std::string& input);
    std::string base64_encode(const std::string& input);

    bool isSocketAlive(int sockfd);

    std::string trim(const std::string& str);

    bool parseHexSize(const std::string& s, size_t& out);

}  // namespace utils
#endif  // HTTP_UTILS_H_