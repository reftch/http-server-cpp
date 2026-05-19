#ifndef LOGGER_H_
#define LOGGER_H_

#include <chrono>
#include <iostream>
#include <string>
#include <string_view>

namespace http {

    class Logger {
       public:
        enum class Level { TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL };

        static Logger& getInstance() {
            static Logger instance;
            return instance;
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
            char timestamp_buffer[20];
            std::string format = "%Y-%m-%d %H:%M:%S";
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
                    level_str = "TRACE";
                    break;
                case Level::DEBUG:
                    level_str = "DEBUG";
                    break;
                case Level::INFO:
                    level_str = "INFO";
                    break;
                case Level::WARNING:
                    level_str = "WARNING";
                    break;
                case Level::ERROR:
                    level_str = "ERROR";
                    break;
                case Level::CRITICAL:
                    level_str = "CRITICAL";
                    break;
            }

            time_t now = time(0);
            struct tm* time_info = localtime(&now);
            std::string timestamp = FormatTime(time_info);

            std::string format_string(format.data(), format.size());
            std::string message = Format(format_string, std::forward<Args>(args)...);
            std::cout << "[" << timestamp << "] [" << level_str << "] " << message << '\n';
        }
    };

}  // namespace http
#endif