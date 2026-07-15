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
     * a JSON-formatted string representation. The resulting string is suitable for
     * use in HTTP responses or logs.
     *
     * @param params The unordered map to convert to JSON.
     * @return A string containing the JSON representation of the input map.
     */
    std::string mapToJson(const std::unordered_map<std::string, std::string>& params);

    /**
     * @brief Reads the contents of a file into a string.
     *
     * Opens a file at the specified path and reads its entire content into a string.
     * If the file cannot be opened or read, an empty string is returned.
     *
     * @param path The file path to read from.
     * @return A string containing the file's contents.
     */
    std::string readFile(const std::string& path);

    /**
     * @brief Decodes a URL-encoded string.
     *
     * This function performs URL decoding on an encoded string, converting
     * percent-encoded characters back to their original form (e.g., "%20" -> " ").
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
     * Supported types: std::string, int, float, double, long, bool.
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

    /**
     * @brief Gets the last modification time of a file.
     *
     * Retrieves the mtime (modification time) of a given file path using `stat()`.
     * If the file does not exist or cannot be accessed, returns 0.
     *
     * @param path The file path to check.
     * @return The modification time as a `time_t`, or 0 if failed.
     */
    time_t getMtime(const std::string& path);

    /**
     * @brief Converts a file's mtime into an HTTP-date formatted string.
     *
     * This function takes a modification time (as returned by `getMtime`) and formats
     * it according to the RFC 7231 specification for HTTP date headers.
     *
     * @param mtime The modification time as a `time_t`.
     * @return An HTTP-date formatted string, e.g., "Wed, 21 Oct 2023 07:28:00 GMT".
     */
    std::string fileMtimeToHttpDate(time_t mtime);

    /**
     * @brief Computes an ETag for a file based on its modification time and size.
     *
     * Generates a unique ETag value for caching purposes, combining the raw mtime
     * and file size in hexadecimal format.
     *
     * @param mtime_raw Raw modification time of the file.
     * @param size Size of the file in bytes.
     * @return A computed ETag string suitable for HTTP response headers.
     */
    std::string computeEtag(int mtime_raw, size_t size);

    /**
     * @brief Converts an integer to a hexadecimal string representation.
     *
     * Converts a number into its lowercase hex string equivalent.
     *
     * @param n The number to convert.
     * @return Hexadecimal string representation of `n`.
     */
    std::string fromIntToHex(size_t n);

    /**
     * @brief Computes the SHA-1 hash of a given input string.
     *
     * Calculates the SHA-1 cryptographic hash of the input and returns it as a
     * hexadecimal string.
     *
     * @param input The input string to hash.
     * @return The SHA-1 hash in lowercase hex format.
     */
    std::string sha1(const std::string& input);

    /**
     * @brief Encodes a string using Base64 encoding.
     *
     * Encodes the input string using standard Base64 algorithm and returns
     * the encoded version.
     *
     * @param input The input string to encode.
     * @return Base64-encoded string.
     */
    std::string base64_encode(const std::string& input);

    /**
     * @brief Checks if a socket is still alive.
     *
     * Performs a basic check on whether a socket descriptor is valid and
     * appears to be connected or usable. It uses `getsockopt` with `SO_ERROR`.
     *
     * @param sockfd The socket file descriptor to test.
     * @return True if the socket is alive, false otherwise.
     */
    bool isSocketAlive(int sockfd);

    /**
     * @brief Trims whitespace from both ends of a string.
     *
     * Removes leading and trailing spaces, tabs, or newlines from a string.
     *
     * @param str The input string to trim.
     * @return Trimmed string with leading/trailing whitespace removed.
     */
    std::string trim(const std::string& str);

    /**
     * @brief Parses a hexadecimal size string into a numeric value.
     *
     * Converts a string that represents a hex number (e.g., "100") into its
     * decimal equivalent. Useful for parsing sizes from HTTP headers like Transfer-Encoding.
     *
     * @param s The input hex string to parse.
     * @param out Output reference where the parsed size will be stored.
     * @return True if parsing succeeded, false otherwise.
     */
    bool parseHexSize(const std::string& s, size_t& out);

}  // namespace utils

#endif  // HTTP_UTILS_H_