#include "logger.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

using namespace http;

class LoggerTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Ensure clean state before each test
        Logger::getInstance().DisableFileLogging();
    }

    void TearDown() override {
        // Clean up after each test
        Logger::getInstance().DisableFileLogging();
        namespace fs = std::filesystem;

        // change this if you want another directory
        fs::path dir{"."};

        // int removed_count = 0;
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".log") {
                if (fs::remove(entry.path())) {
                    std::cout << "Removed: " << entry.path() << '\n';
                } else {
                    std::cout << "Failed to remove: " << entry.path() << '\n';
                }
            }
        }
    }
};

TEST_F(LoggerTest, SingletonInstance) {
    Logger& logger1 = Logger::getInstance();
    Logger& logger2 = Logger::getInstance();
    EXPECT_EQ(&logger1, &logger2);
}

TEST_F(LoggerTest, EnableFileLoggingWithPath) {
    Logger& logger = Logger::getInstance();
    logger.EnableFileLogging("./test.log");
    EXPECT_TRUE(logger.IsFileLoggingEnabled());
    EXPECT_EQ(logger.GetLogFilePath(), "./test.log");
}

TEST_F(LoggerTest, EnableFileLoggingDefaultPath) {
    Logger& logger = Logger::getInstance();
    logger.EnableFileLogging();  // Should use default path
    EXPECT_TRUE(logger.IsFileLoggingEnabled());
    EXPECT_EQ(logger.GetLogFilePath(), "./http-server.log");
}

TEST_F(LoggerTest, DisableFileLogging) {
    Logger& logger = Logger::getInstance();
    logger.EnableFileLogging("./test.log");
    EXPECT_TRUE(logger.IsFileLoggingEnabled());

    logger.DisableFileLogging();
    EXPECT_FALSE(logger.IsFileLoggingEnabled());
}

TEST_F(LoggerTest, LogToFileAndConsole) {
    const std::string test_file = "./log_test.log";
    Logger& logger = Logger::getInstance();
    logger.EnableFileLogging(test_file);

    // Capture console output (optional, but not directly testable in gtest)
    logger.Info("Test message: {}", 42);

    // Check file exists and has content
    std::ifstream file(test_file);
    EXPECT_TRUE(file.is_open());

    std::string line;
    std::getline(file, line);
    EXPECT_NE(line, "");

    file.close();
    // std::filesystem::remove(test_file);
}

TEST_F(LoggerTest, LogLevels) {
    const std::string test_file = "./levels_test.log";
    Logger& logger = Logger::getInstance();
    logger.EnableFileLogging(test_file);

    logger.Trace("Trace message");
    logger.Debug("Debug message");
    logger.Info("Info message");
    logger.Warning("Warning message");
    logger.Error("Error message");
    logger.Critical("Critical message");

    std::ifstream file(test_file);
    EXPECT_TRUE(file.is_open());

    std::string line;
    int count = 0;
    while (std::getline(file, line)) {
        count++;
        EXPECT_NE(line, "");
    }
    EXPECT_EQ(count, 6);  // 6 log lines

    file.close();
}
