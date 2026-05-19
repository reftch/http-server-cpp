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

        class LogStream {
           public:
            LogStream(Logger& logger, Level level) : logger_(logger), level_(level) {}

            template <typename... Args>
            void operator()(std::string_view format, Args&&... args) {
                logger_.log(level_, format, std::forward<Args>(args)...);
            }

           private:
            Logger& logger_;
            Level level_;
        };

        static Logger& getInstance() {
            static Logger instance;
            return instance;
        }

        LogStream trace() { return LogStream(*this, Level::TRACE); }
        LogStream debug() { return LogStream(*this, Level::DEBUG); }
        LogStream info() { return LogStream(*this, Level::INFO); }
        LogStream warning() { return LogStream(*this, Level::WARNING); }
        LogStream error() { return LogStream(*this, Level::ERROR); }
        LogStream critical() { return LogStream(*this, Level::CRITICAL); }

       private:
        Logger() = default;

        std::string format_time_with_strftime_manual(const struct tm* time_info) {
            char timestamp_buffer[20];

            // Format string
            std::string format = "%Y-%m-%d %H:%M:%S";

            // Use strftime to format the time into the buffer
            if (std::strftime(timestamp_buffer, sizeof(timestamp_buffer), format.c_str(), time_info) == 0) {
                return "";
            }

            std::string result;
            size_t length = std::strlen(timestamp_buffer);
            result.assign(timestamp_buffer, length);

            return result;
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
        void log(Level level, std::string_view format, Args&&... args) {
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
            std::string timestamp = format_time_with_strftime_manual(time_info);

            std::string format_string(format.data(), format.size());
            std::string message = Format(format_string, std::forward<Args>(args)...);
            std::cout << "[" << timestamp << "] [" << level_str << "] " << message << '\n';
        }
    };

}  // namespace http
#endif