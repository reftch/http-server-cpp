#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

#include "utils.h"

// Test fixture for environment variable manipulation
class GetEnvTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Store original environment variables
        original_value = std::getenv("TEST_VAR");
        if (original_value) {
            original_value_str = std::string(original_value);
        }
        // Clear the test variable before each test
        unsetenv("TEST_VAR");
    }

    void TearDown() override {
        // Restore original environment variable
        if (original_value) {
            setenv("TEST_VAR", original_value_str.c_str(), 1);
        } else {
            unsetenv("TEST_VAR");
        }
    }

   private:
    const char* original_value = nullptr;
    std::string original_value_str;
};

// Test with string type
TEST_F(GetEnvTest, GetStringDefaultValue) {
    // Test when environment variable is not set
    std::string result = GetEnv<std::string>("TEST_VAR", "default");
    EXPECT_EQ(result, "default");
}

TEST_F(GetEnvTest, GetStringFromEnvironment) {
    // Test when environment variable is set
    setenv("TEST_VAR", "hello_world", 1);
    std::string result = GetEnv<std::string>("TEST_VAR", "default");
    EXPECT_EQ(result, "hello_world");
}

// Test with int type
TEST_F(GetEnvTest, GetIntDefaultValue) {
    // Test when environment variable is not set
    int result = GetEnv<int>("TEST_VAR", 42);
    EXPECT_EQ(result, 42);
}

TEST_F(GetEnvTest, GetIntFromEnvironment) {
    // Test when environment variable is set
    setenv("TEST_VAR", "123", 1);
    int result = GetEnv<int>("TEST_VAR", 42);
    EXPECT_EQ(result, 123);
}

TEST_F(GetEnvTest, GetIntInvalidValueReturnsDefault) {
    // Test with invalid numeric value
    setenv("TEST_VAR", "not_a_number", 1);
    int result = GetEnv<int>("TEST_VAR", 42);
    EXPECT_EQ(result, 42);  // Should return default
}

// Test with long type
TEST_F(GetEnvTest, GetLongDefaultValue) {
    long result = GetEnv<long>("TEST_VAR", 999L);
    EXPECT_EQ(result, 999L);
}

TEST_F(GetEnvTest, GetLongFromEnvironment) {
    setenv("TEST_VAR", "987654321", 1);
    long result = GetEnv<long>("TEST_VAR", 999L);
    EXPECT_EQ(result, 987654321L);
}

// Test with bool type - true values
TEST_F(GetEnvTest, GetBoolTrueValues) {
    // Test various true representations
    setenv("TEST_VAR", "true", 1);
    bool result = GetEnv<bool>("TEST_VAR", false);
    EXPECT_TRUE(result);

    setenv("TEST_VAR", "TRUE", 1);
    result = GetEnv<bool>("TEST_VAR", false);
    EXPECT_TRUE(result);

    setenv("TEST_VAR", "1", 1);
    result = GetEnv<bool>("TEST_VAR", false);
    EXPECT_TRUE(result);

    setenv("TEST_VAR", "yes", 1);
    result = GetEnv<bool>("TEST_VAR", false);
    EXPECT_TRUE(result);
}

// Test with bool type - false values
TEST_F(GetEnvTest, GetBoolFalseValues) {
    // Test various false representations
    setenv("TEST_VAR", "false", 1);
    bool result = GetEnv<bool>("TEST_VAR", true);
    EXPECT_FALSE(result);

    setenv("TEST_VAR", "FALSE", 1);
    result = GetEnv<bool>("TEST_VAR", true);
    EXPECT_FALSE(result);

    setenv("TEST_VAR", "0", 1);
    result = GetEnv<bool>("TEST_VAR", true);
    EXPECT_FALSE(result);

    setenv("TEST_VAR", "no", 1);
    result = GetEnv<bool>("TEST_VAR", true);
    EXPECT_FALSE(result);
}

// Test with bool type - invalid values return default
TEST_F(GetEnvTest, GetBoolInvalidValueReturnsDefault) {
    // Test with invalid boolean value
    setenv("TEST_VAR", "maybe", 1);
    bool result = GetEnv<bool>("TEST_VAR", true);
    EXPECT_TRUE(result);  // Should return default (true)

    // Test with empty string
    setenv("TEST_VAR", "", 1);
    result = GetEnv<bool>("TEST_VAR", false);
    EXPECT_FALSE(result);  // Should return default (false)
}

// Test with bool type - environment variable not set
TEST_F(GetEnvTest, GetBoolNotSetReturnsDefault) {
    bool result = GetEnv<bool>("NONEXISTENT_VAR", true);
    EXPECT_TRUE(result);  // Should return default (true)

    result = GetEnv<bool>("NONEXISTENT_VAR", false);
    EXPECT_FALSE(result);  // Should return default (false)
}

// Test with float type
TEST_F(GetEnvTest, GetFloatDefaultValue) {
    float result = GetEnv<float>("TEST_VAR", 3.14f);
    EXPECT_FLOAT_EQ(result, 3.14f);
}

TEST_F(GetEnvTest, GetFloatFromEnvironment) {
    setenv("TEST_VAR", "2.718", 1);
    float result = GetEnv<float>("TEST_VAR", 3.14f);
    EXPECT_FLOAT_EQ(result, 2.718f);
}

// Test with double type
TEST_F(GetEnvTest, GetDoubleDefaultValue) {
    double result = GetEnv<double>("TEST_VAR", 3.14159);
    EXPECT_DOUBLE_EQ(result, 3.14159);
}

TEST_F(GetEnvTest, GetDoubleFromEnvironment) {
    setenv("TEST_VAR", "2.718281828", 1);
    double result = GetEnv<double>("TEST_VAR", 3.14159);
    EXPECT_DOUBLE_EQ(result, 2.718281828);
}

// Test with negative numbers
TEST_F(GetEnvTest, GetNegativeNumbers) {
    setenv("TEST_VAR", "-42", 1);
    int result = GetEnv<int>("TEST_VAR", 0);
    EXPECT_EQ(result, -42);

    setenv("TEST_VAR", "-3.14", 1);
    double result_double = GetEnv<double>("TEST_VAR", 0.0);
    EXPECT_DOUBLE_EQ(result_double, -3.14);
}

// Test with large numbers
TEST_F(GetEnvTest, GetLargeNumbers) {
    setenv("TEST_VAR", "9223372036854775807", 1);  // max long long
    long long result = GetEnv<long long>("TEST_VAR", 0);
    EXPECT_EQ(result, 9223372036854775807LL);
}

// Test edge cases for boolean values
TEST_F(GetEnvTest, GetBoolEdgeCases) {
    // Test whitespace handling (should be handled by std::string conversion)
    setenv("TEST_VAR", " true ", 1);  // leading/trailing spaces
    bool result = GetEnv<bool>("TEST_VAR", false);
    EXPECT_FALSE(result);  // Should not match "true" due to spaces

    // Test empty string
    setenv("TEST_VAR", "", 1);
    result = GetEnv<bool>("TEST_VAR", true);
    EXPECT_TRUE(result);  // Should return default
}

// Test with different numeric types
TEST_F(GetEnvTest, GetDifferentNumericTypes) {
    // Test short
    setenv("TEST_VAR", "100", 1);
    short result_short = GetEnv<short>("TEST_VAR", 0);
    EXPECT_EQ(result_short, 100);

    // Test unsigned int
    setenv("TEST_VAR", "500", 1);
    unsigned int result_uint = GetEnv<unsigned int>("TEST_VAR", 0);
    EXPECT_EQ(result_uint, 500U);

    // Test char
    setenv("TEST_VAR", "65", 1);
    char result_char = GetEnv<char>("TEST_VAR", 'a');
    EXPECT_EQ(result_char, 'A');  // ASCII 65 is 'A'
}
