#ifndef LOGGER_H_
#define LOGGER_H_

#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace http {

    class Logger {
       private:
        enum class Level { TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL };

        bool file_logging_enabled_ = false;
        std::ofstream log_file_;
        std::string log_file_path_;
        mutable std::mutex log_mutex_;

       public:
        static Logger& getInstance() {
            static Logger instance;
            return instance;
        }

        // Enable file logging with specific file path and name
        void EnableFileLogging(const std::string& filepath) {
            std::lock_guard<std::mutex> lock(log_mutex_);
            if (log_file_.is_open()) {
                log_file_.close();
            }

            log_file_path_ = filepath;
            log_file_.open(filepath, std::ios::app);
            file_logging_enabled_ = log_file_.is_open();
        }

        // Enable file logging with default path (current directory)
        // void EnableFileLogging() { EnableFileLogging("http-server.log"); }

        void DisableFileLogging() {
            std::lock_guard<std::mutex> lock(log_mutex_);
            file_logging_enabled_ = false;
            if (log_file_.is_open()) {
                log_file_.close();
            }
        }

        // Get current log file path
        std::string GetLogFilePath() const {
            std::lock_guard<std::mutex> lock(log_mutex_);
            return log_file_path_;
        }

        // Check if file logging is enabled
        bool IsFileLoggingEnabled() const {
            std::lock_guard<std::mutex> lock(log_mutex_);
            return file_logging_enabled_;
        }

        template <typename... Args>
        void Trace(std::string_view format, Args&&... args) {
            Log(Level::TRACE, format, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void Debug(std::string_view format, Args&&... args) {
            Log(Level::DEBUG, format, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void Info(std::string_view format, Args&&... args) {
            Log(Level::INFO, format, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void Warning(std::string_view format, Args&&... args) {
            Log(Level::WARNING, format, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void Error(std::string_view format, Args&&... args) {
            Log(Level::ERROR, format, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void Critical(std::string_view format, Args&&... args) {
            Log(Level::CRITICAL, format, std::forward<Args>(args)...);
        }

       private:
        Logger() = default;

        std::string FormatTime(const struct tm* time_info) {
            char timestamp_buffer[23];
            std::string format = "[%Y-%m-%d %H:%M:%S] ";
            if (std::strftime(timestamp_buffer, sizeof(timestamp_buffer), format.c_str(), time_info) == 0) {
                return "";
            }
            return std::string(timestamp_buffer);
        }

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

        template <typename... Args>
        void Log(Level level, std::string_view format, Args&&... args) {
            std::string level_str;
            switch (level) {
                case Level::TRACE:
                    level_str = "[TRACE]";
                    break;
                case Level::DEBUG:
                    level_str = "[DEBUG]";
                    break;
                case Level::INFO:
                    level_str = "[INFO]";
                    break;
                case Level::WARNING:
                    level_str = "[WARNING]";
                    break;
                case Level::ERROR:
                    level_str = "[ERROR]";
                    break;
                case Level::CRITICAL:
                    level_str = "[CRITICAL]";
                    break;
            }

            time_t now = time(0);
            struct tm* time_info = localtime(&now);
            std::string timestamp = FormatTime(time_info);

            std::string format_string(format.data(), format.size());
            std::string message = Format(format_string, std::forward<Args>(args)...);
            std::string full_message = timestamp + level_str + message + '\n';

            // Console logging
            std::cout << full_message;

            // File logging (if enabled)
            if (file_logging_enabled_ && log_file_.is_open()) {
                log_file_ << full_message;
                log_file_.flush();
            }
        }
    };

}  // namespace http
#endif