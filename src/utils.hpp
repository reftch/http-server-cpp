#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

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

#endif // UTILS_HPP