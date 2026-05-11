#ifndef SERVER_TEST_HPP
#define SERVER_TEST_HPP

#include "gtest/gtest.h"
#include "server.hpp"

using namespace http;

// Define a fixture class to hold setup logic and shared objects
class ServerTestFixture : public ::testing::Test {
   protected:
    // Define standard test parameters
    const std::string test_host = "127.0.0.1";
    const int test_port = 8080;

    // The object under test, managed by smart pointer
    std::unique_ptr<server> server_;

    // Helper function to create a simple handler for testing
    static response_body dummy_handler(const std::string& path, const auto&) { return "Test Response for " + path; }

    // SetUp runs before EVERY test in this fixture
    void SetUp() override {
        // Instantiate the server object before each test runs
        server_ = std::make_unique<server>(test_host, test_port);
    }
};

#endif