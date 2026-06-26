/**
 * @file logger.h
 * @brief HTTP Server Logger class for logging messages to console and file.
 *
 * This header file defines the Logger class used for logging messages in an HTTP server.
 * It supports different log levels, console output, and optional file logging with thread safety.
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace http {
    /**
     * @enum Level
     * @brief Log levels used for categorizing log messages.
     */
    enum class Level { TRACE = 0, DEBUG = 1, INFO = 2, WARNING = 3, ERROR = 4, CRITICAL = 5 };

    /**
     * @class Logger
     * @brief Thread-safe logger for HTTP server with console and file output support.
     *
     * The Logger class provides a singleton instance that can log messages at different levels:
     * TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL.
     * It supports both console logging and optional file logging with thread-safe operations.
     */
    class Logger {
       private:
        bool file_logging_enabled_ = false; /**< Flag indicating if file logging is enabled. */
        std::ofstream log_file_;            /**< Output file stream for log file. */
        std::string log_file_path_;         /**< Path to the current log file. */
        mutable std::mutex log_mutex_;      /**< Mutex for thread-safe operations. */
        Level current_level = Level::INFO;

       public:
        /**
         * @brief Get the singleton instance of Logger.
         * @return Reference to the Logger singleton instance.
         */
        static Logger& getInstance() {
            static Logger instance;
            return instance;
        }

        void setLevel(Level level) { current_level = level; }

        /**
         * @brief Enable file logging with a specific file path.
         * @param filepath Path to the log file where messages will be written.
         *
         * This method opens the specified file for logging. If the file is already open,
         * it will be closed and reopened with the new path.
         * File logging will be disabled if the file cannot be opened.
         */
        void enableFileLogging(const std::string& filepath) {
            std::lock_guard<std::mutex> lock(log_mutex_);
            if (log_file_.is_open()) {
                log_file_.close();
            }

            log_file_path_ = filepath;
            log_file_.open(filepath, std::ios::app);
            file_logging_enabled_ = log_file_.is_open();
        }

        /**
         * @brief Enable file logging with default path.
         *
         * This method enables file logging using the default path "./http-server.log".
         * The log file will be created in the current working directory.
         */
        void enableFileLogging() { enableFileLogging("./http-server.log"); }

        /**
         * @brief Disable file logging.
         *
         * This method closes any open log file and disables file logging.
         * Console logging will continue to work normally.
         */
        void disableFileLogging() {
            std::lock_guard<std::mutex> lock(log_mutex_);
            file_logging_enabled_ = false;
            if (log_file_.is_open()) {
                log_file_.close();
            }
        }

        /**
         * @brief Get the current log file path.
         * @return Current log file path as a string.
         */
        std::string getLogFilePath() const {
            std::lock_guard<std::mutex> lock(log_mutex_);
            return log_file_path_;
        }

        /**
         * @brief Check if file logging is enabled.
         * @return True if file logging is enabled, false otherwise.
         */
        bool IsFileLoggingEnabled() const {
            std::lock_guard<std::mutex> lock(log_mutex_);
            return file_logging_enabled_;
        }

        /**
         * @brief Log a TRACE level message.
         * @param format Format string with placeholders for arguments.
         * @param args Arguments to be formatted into the message.
         *
         * TRACE level messages are used for detailed debugging information.
         */
        template <typename... Args>
        void trace(std::string_view format, Args&&... args) {
            if (current_level <= Level::TRACE) {
                log(Level::TRACE, format, std::forward<Args>(args)...);
            }
        }

        /**
         * @brief Log a DEBUG level message.
         * @param format Format string with placeholders for arguments.
         * @param args Arguments to be formatted into the message.
         *
         * DEBUG level messages are used for detailed debugging information.
         */
        template <typename... Args>
        void debug(std::string_view format, Args&&... args) {
            if (current_level <= Level::DEBUG) {
                log(Level::DEBUG, format, std::forward<Args>(args)...);
            }
        }

        /**
         * @brief Log an INFO level message.
         * @param format Format string with placeholders for arguments.
         * @param args Arguments to be formatted into the message.
         *
         * INFO level messages provide general information about program execution.
         */
        template <typename... Args>
        void info(std::string_view format, Args&&... args) {
            if (current_level <= Level::INFO) {
                log(Level::INFO, format, std::forward<Args>(args)...);
            }
        }

        /**
         * @brief Log a WARNING level message.
         * @param format Format string with placeholders for arguments.
         * @param args Arguments to be formatted into the message.
         *
         * WARNING level messages indicate potential problems that don't stop execution.
         */
        template <typename... Args>
        void warning(std::string_view format, Args&&... args) {
            if (current_level <= Level::WARNING) {
                log(Level::WARNING, format, std::forward<Args>(args)...);
            }
        }

        /**
         * @brief Log an ERROR level message.
         * @param format Format string with placeholders for arguments.
         * @param args Arguments to be formatted into the message.
         *
         * ERROR level messages indicate errors that occurred during execution.
         */
        template <typename... Args>
        void error(std::string_view format, Args&&... args) {
            if (current_level <= Level::ERROR) {
                log(Level::ERROR, format, std::forward<Args>(args)...);
            }
        }

        /**
         * @brief Log a CRITICAL level message.
         * @param format Format string with placeholders for arguments.
         * @param args Arguments to be formatted into the message.
         *
         * CRITICAL level messages indicate serious errors that may cause program termination.
         */
        template <typename... Args>
        void critical(std::string_view format, Args&&... args) {
            if (current_level <= Level::CRITICAL) {
                log(Level::CRITICAL, format, std::forward<Args>(args)...);
            }
        }

        std::string toString() {
            switch (current_level) {
                case Level::TRACE:
                    return "TRACE";
                case Level::DEBUG:
                    return "DEBUG";
                case Level::INFO:
                    return "INFO";
                case Level::WARNING:
                    return "WARNING";
                case Level::ERROR:
                    return "ERROR";
                case Level::CRITICAL:
                    return "CRITICAL";
                default:
                    return "UNKNOWN";
            }
        }

       protected:
        /**
         * @brief Default constructor for Logger.
         *
         * The constructor is protected to ensure that only the singleton instance can be created.
         */
        Logger() = default;

        /**
         * @brief Format timestamp into a readable string.
         * @param time_info Pointer to struct tm containing time information.
         * @return Formatted timestamp string.
         */
        std::string formatTime(const struct tm* time_info) {
            char timestamp_buffer[23];
            std::string format = "[%Y-%m-%d %H:%M:%S] ";
            if (std::strftime(timestamp_buffer, sizeof(timestamp_buffer), format.c_str(), time_info) == 0) {
                return "";
            }
            return std::string(timestamp_buffer);
        }

        /**
         * @brief Format a message string with provided arguments.
         * @param fmt Format string containing {} placeholders.
         * @param args Arguments to be inserted into the format string.
         * @return Formatted message string.
         */
        template <typename... Args>
        std::string Format(const std::string& fmt, Args... args) {
            std::string result = fmt;
            std::vector<std::string> args_vector{[&args]() {
                std::ostringstream oss;
                oss << args;
                return oss.str();
            }()...};

            size_t pos = 0;
            for (const auto& arg : args_vector) {
                pos = result.find("{}");
                if (pos != std::string::npos) {
                    result.replace(pos, 2, arg);
                }
            }

            return result;
        }

        /**
         * @brief Internal logging method that handles message formatting and output.
         * @param level Log level for the message.
         * @param format Format string with placeholders for arguments.
         * @param args Arguments to be formatted into the message.
         */
        template <typename... Args>
        void log(Level level, std::string_view format, Args&&... args) {
            std::string level_str;
            switch (level) {
                case Level::TRACE:
                    level_str = "[TRACE] ";
                    break;
                case Level::DEBUG:
                    level_str = "[DEBUG] ";
                    break;
                case Level::INFO:
                    level_str = "[INFO] ";
                    break;
                case Level::WARNING:
                    level_str = "[WARNING] ";
                    break;
                case Level::ERROR:
                    level_str = "[ERROR] ";
                    break;
                case Level::CRITICAL:
                    level_str = "[CRITICAL] ";
                    break;
            }

            time_t now = time(0);
            struct tm* time_info = localtime(&now);
            std::string timestamp = formatTime(time_info);

            std::string format_string(format.data(), format.size());
            std::string message = Format(format_string, std::forward<Args>(args)...);

            std::string full_message = timestamp + level_str + message + '\n';

            // Fast console output (no iostream)
            std::fwrite(full_message.data(), 1, full_message.size(), stdout);
            std::fflush(stdout);

            // file logging
            if (file_logging_enabled_ && log_file_.is_open()) {
                log_file_ << full_message;
                log_file_.flush();
            }
        }
    };

}  // namespace http
#endif  // LOGGER_H_