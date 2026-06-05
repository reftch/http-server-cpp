#ifndef HTTP_UTILS_H_
#define HTTP_UTILS_H_

#include <ctime>  // For time functions
#include <fstream>
#include <iomanip>  // For stream manipulation (like std::put_time)
#include <iostream>
#include <map>
#include <optional>
#include <sstream>  // Must include this for stringstream
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @file utils.hpp
 * @brief Utility functions for system information and help messages.
 */

/**
 * @brief Gets the current system build date in YYYY-MM-DD format.
 * @return A string representing the current date.
 */
std::string get_build_date();

/**
 * @brief Prints the application's usage instructions and available options.
 *
 * This function displays the help message to the user.
 */
void print_help();

/**
 * @brief Parses command-line arguments for host and port configuration.
 *
 * This function processes command-line arguments and returns a map containing
 * the host and port values. If no arguments are provided, default values are used:
 * - Host: "0.0.0.0"
 * - Port: "8080"
 *
 * @param argc The number of command-line arguments
 * @param argv An array of command-line argument strings
 * @return A map containing the parsed arguments with keys "--host" and "--port"
 *
 * @example
 * parseArguments(3, {"server", "--host", "127.0.0.1"})
 * // Returns map with "--host": "127.0.0.1" and "--port": "8080"
 *
 * @example
 * parseArguments(5, {"server", "--port", "9000", "--host", "192.168.1.1"})
 * // Returns map with "--host": "192.168.1.1" and "--port": "9000"
 */
std::map<std::string, std::string> ParseArguments(int argc, char* argv[]);

std::string MapToJson(const std::unordered_map<std::string, std::string>& params);

std::string ReadFile(const std::string& path);

std::string UrlDecode(const std::string& encoded);

std::optional<std::string> GetEnv(const std::string& key);

#endif  // UTILS_HPP