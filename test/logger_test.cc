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
   public:
   protected:
    void SetUp() override {
        // Ensure clean state before each test
        Logger::getInstance().disableFileLogging();
    }

    void TearDown() override {
        // Clean up after each test
        Logger::getInstance().disableFileLogging();
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
    logger.enableFileLogging("./test.log");
    EXPECT_TRUE(logger.IsFileLoggingEnabled());
    EXPECT_EQ(logger.getLogFilePath(), "./test.log");
}

TEST_F(LoggerTest, EnableFileLoggingDefaultPath) {
    Logger& logger = Logger::getInstance();
    logger.enableFileLogging();  // Should use default path
    EXPECT_TRUE(logger.IsFileLoggingEnabled());
    EXPECT_EQ(logger.getLogFilePath(), "./http-server.log");
}

TEST_F(LoggerTest, DisableFileLogging) {
    Logger& logger = Logger::getInstance();
    logger.enableFileLogging("./test.log");
    EXPECT_TRUE(logger.IsFileLoggingEnabled());

    logger.disableFileLogging();
    EXPECT_FALSE(logger.IsFileLoggingEnabled());
}

TEST_F(LoggerTest, LogToFileAndConsole) {
    const std::string test_file = "./log_test.log";
    Logger& logger = Logger::getInstance();
    logger.enableFileLogging(test_file);

    // Capture console output (optional, but not directly testable in gtest)
    logger.info("Test message: {}", 42);

    // Check file exists and has content
    std::ifstream file(test_file);
    EXPECT_TRUE(file.is_open());

    std::string line;
    std::getline(file, line);
    EXPECT_NE(line, "");

    file.close();
}

TEST_F(LoggerTest, LogLevelsDefault) {
    const std::string test_file = "./levels_test.log";
    Logger& logger = Logger::getInstance();
    logger.enableFileLogging(test_file);

    logger.trace("Trace message");
    logger.debug("Debug message");
    logger.info("Info message");
    logger.warning("Warning message");
    logger.error("Error message");
    logger.critical("Critical message");

    std::ifstream file(test_file);
    EXPECT_TRUE(file.is_open());

    std::string line;
    int count = 0;
    while (std::getline(file, line)) {
        count++;
        EXPECT_NE(line, "");
    }
    EXPECT_EQ(count, 4);  // 4 log lines

    file.close();
}

TEST_F(LoggerTest, LogLevelsTrace) {
    const std::string test_file = "./levels_test.log";
    Logger& logger = Logger::getInstance();
    logger.enableFileLogging(test_file);

    logger.setLevel(Level::TRACE);

    logger.trace("Trace message");
    logger.debug("Debug message");
    logger.info("Info message");
    logger.warning("Warning message");
    logger.error("Error message");
    logger.critical("Critical message");

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

TEST_F(LoggerTest, LogLevelsError) {
    const std::string test_file = "./levels_test.log";
    Logger& logger = Logger::getInstance();
    logger.enableFileLogging(test_file);

    logger.setLevel(Level::ERROR);

    logger.trace("Trace message");
    logger.debug("Debug message");
    logger.info("Info message");
    logger.warning("Warning message");
    logger.error("Error message");
    logger.critical("Critical message");

    std::ifstream file(test_file);
    EXPECT_TRUE(file.is_open());

    std::string line;
    int count = 0;
    while (std::getline(file, line)) {
        count++;
        EXPECT_NE(line, "");
    }
    EXPECT_EQ(count, 2);

    file.close();
}

class LoggerForTest : public Logger {
   public:
    using Logger::Format;
    using Logger::formatTime;
};

TEST_F(LoggerTest, FormatTime) {
    LoggerForTest logger;
    struct tm time_info = {};
    time_info.tm_year = 2026 - 1900;  // Year since 1900
    time_info.tm_mon = 5 - 1;         // Month (0–11)
    time_info.tm_mday = 15;
    time_info.tm_hour = 10;
    time_info.tm_min = 30;
    time_info.tm_sec = 45;

    std::string expected = "[2026-05-15 10:30:45] ";
    std::string actual = logger.formatTime(&time_info);

    EXPECT_EQ(expected, actual);
}

// Test edge case: January (month 0)
TEST_F(LoggerTest, FormatTimeJanuary) {
    LoggerForTest logger;
    struct tm time_info = {};
    time_info.tm_year = 2026 - 1900;
    time_info.tm_mon = 0;  // January
    time_info.tm_mday = 1;
    time_info.tm_hour = 0;
    time_info.tm_min = 0;
    time_info.tm_sec = 0;

    std::string expected = "[2026-01-01 00:00:00] ";
    std::string actual = logger.formatTime(&time_info);

    EXPECT_EQ(expected, actual);
}

// Test edge case: Just before midnight (23:59:59)
TEST_F(LoggerTest, FormatTimeJustBeforeMidnight) {
    LoggerForTest logger;
    struct tm time_info = {};
    time_info.tm_year = 2026 - 1900;
    time_info.tm_mon = 12 - 1;  // December
    time_info.tm_mday = 31;
    time_info.tm_hour = 23;
    time_info.tm_min = 59;
    time_info.tm_sec = 59;

    std::string expected = "[2026-12-31 23:59:59] ";
    std::string actual = logger.formatTime(&time_info);

    EXPECT_EQ(expected, actual);
}

TEST_F(LoggerTest, FormatBasicReplacement) {
    LoggerForTest logger;
    std::string result = logger.Format("Hello {}!", "World");
    EXPECT_EQ("Hello World!", result);
}

TEST_F(LoggerTest, FormatMultipleReplacements) {
    LoggerForTest logger;
    std::string result = logger.Format("Hello {}, you are {} years old", "Alice", 25);
    EXPECT_EQ("Hello Alice, you are 25 years old", result);
}

TEST_F(LoggerTest, FormatNoPlaceholders) {
    LoggerForTest logger;
    std::string result = logger.Format("Hello World!");
    EXPECT_EQ("Hello World!", result);
}

TEST_F(LoggerTest, FormatEmptyString) {
    LoggerForTest logger;
    std::string result = logger.Format("");
    EXPECT_EQ("", result);
}

TEST_F(LoggerTest, FormatEmptyPlaceholder) {
    LoggerForTest logger;
    std::string result = logger.Format("Hello {} World", "");
    EXPECT_EQ("Hello  World", result);
}

TEST_F(LoggerTest, FormatMultipleSamePlaceholders) {
    LoggerForTest logger;
    std::string result = logger.Format("{} {} {}", "A", "B", "C");
    EXPECT_EQ("A B C", result);
}

TEST_F(LoggerTest, FormatSingleCharacter) {
    LoggerForTest logger;
    std::string result = logger.Format("Hello {}!", 'X');
    EXPECT_EQ("Hello X!", result);
}

TEST_F(LoggerTest, FormatInteger) {
    LoggerForTest logger;
    std::string result = logger.Format("The number is {}", 42);
    EXPECT_EQ("The number is 42", result);
}

TEST_F(LoggerTest, FormatFloat) {
    LoggerForTest logger;
    std::string result = logger.Format("Pi is approximately {}", 3.14159);
    EXPECT_EQ("Pi is approximately 3.14159", result);
}

TEST_F(LoggerTest, FormatDouble) {
    LoggerForTest logger;
    std::string result = logger.Format("Value: {}", 2.718281828);
    EXPECT_EQ("Value: 2.71828", result);
}

TEST_F(LoggerTest, FormatStringWithSpecialCharacters) {
    LoggerForTest logger;
    std::string result = logger.Format("Message: {}", "Hello\nWorld\t!");
    EXPECT_EQ("Message: Hello\nWorld\t!", result);
}

TEST_F(LoggerTest, FormatStringWithSpaces) {
    LoggerForTest logger;
    std::string result = logger.Format("Hello {} World", "Beautiful");
    EXPECT_EQ("Hello Beautiful World", result);
}

TEST_F(LoggerTest, FormatStringWithNumbers) {
    LoggerForTest logger;
    std::string result = logger.Format("Score: {} points", 100);
    EXPECT_EQ("Score: 100 points", result);
}

TEST_F(LoggerTest, FormatComplexString) {
    LoggerForTest logger;
    std::string result = logger.Format("User {} has {} points and is {} years old", "John", 150, 25);
    EXPECT_EQ("User John has 150 points and is 25 years old", result);
}

TEST_F(LoggerTest, FormatWithMorePlaceholdersThanArgs) {
    LoggerForTest logger;
    std::string result = logger.Format("Hello {} {} {}", "World", "Test");
    // Should replace only the available placeholders
    EXPECT_EQ("Hello World Test {}", result);  // Last {} remains empty
}

TEST_F(LoggerTest, FormatWithMoreArgsThanPlaceholders) {
    LoggerForTest logger;
    std::string result = logger.Format("Hello {}", "World", "Extra", "Arguments");
    EXPECT_EQ("Hello World", result);  // Only first placeholder replaced
}

TEST_F(LoggerTest, FormatWithMixedTypes) {
    LoggerForTest logger;
    std::string result = logger.Format("Name: {}, Age: {}, Height: {}m", "Alice", 30, 5.5);
    EXPECT_EQ("Name: Alice, Age: 30, Height: 5.5m", result);
}

TEST_F(LoggerTest, FormatWithZeroValues) {
    LoggerForTest logger;
    std::string result = logger.Format("Zero values: {}, {}, {}", 0, 0.0, false);
    EXPECT_EQ("Zero values: 0, 0, 0", result);
}

TEST_F(LoggerTest, FormatWithBooleanValues) {
    LoggerForTest logger;
    std::string result = logger.Format("Boolean values: {}, {}", true, false);
    EXPECT_EQ("Boolean values: 1, 0", result);
}

TEST_F(LoggerTest, FormatWithNegativeNumbers) {
    LoggerForTest logger;
    std::string result = logger.Format("Negative values: {}, {}", -42, -3.14);
    EXPECT_EQ("Negative values: -42, -3.14", result);
}

TEST_F(LoggerTest, FormatWithLeadingZeros) {
    LoggerForTest logger;
    std::string result = logger.Format("Number: {}", 007);
    EXPECT_EQ("Number: 7", result);
}

TEST_F(LoggerTest, FormatWithScientificNotation) {
    LoggerForTest logger;
    std::string result = logger.Format("Scientific: {}", 1.23e-4);
    EXPECT_EQ("Scientific: 0.000123", result);
}

TEST_F(LoggerTest, FormatWithLongString) {
    LoggerForTest logger;
    std::string long_string =
        "This is a very long string that should be properly formatted with the placeholder replacement mechanism";
    std::string result = logger.Format("Long message: {}", long_string);
    EXPECT_EQ("Long message: " + long_string, result);
}

TEST_F(LoggerTest, FormatWithSpecialFormatCharacters) {
    LoggerForTest logger;
    std::string result = logger.Format("Special chars: {} {} {}", "Hello", "World", "!");
    EXPECT_EQ("Special chars: Hello World !", result);
}

TEST_F(LoggerTest, FormatWithPercentSign) {
    LoggerForTest logger;
    std::string result = logger.Format("Percentage: {}%", 95);
    EXPECT_EQ("Percentage: 95%", result);
}

TEST_F(LoggerTest, FormatWithMultipleConsecutivePlaceholders) {
    LoggerForTest logger;
    std::string result = logger.Format("A{}B{}C{}D", "X", "Y", "Z");
    EXPECT_EQ("AXBYCZD", result);
}

TEST_F(LoggerTest, FormatWithEmptyArgs) {
    LoggerForTest logger;
    std::string result = logger.Format("{}", "");
    EXPECT_EQ("", result);
}

TEST_F(LoggerTest, FormatWithNullptr) {
    LoggerForTest logger;
    // This test assumes the implementation handles nullptr gracefully
    // The behavior may vary depending on how std::ostringstream handles nullptr
    std::string result = logger.Format("{}", nullptr);
    EXPECT_EQ("nullptr", result);
}