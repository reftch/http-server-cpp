#ifndef SERVER_TEST_HPP
#define SERVER_TEST_HPP

#include "gtest/gtest.h"
#include "server.h"

// Define a fixture class to hold setup logic and shared objects
class ServerTestFixture : public ::testing::Test {
   protected:
    // Define standard test parameters
    const std::string test_host = "127.0.0.1";
    const int test_port = 8080;

    // The object under test, managed by smart pointer
    std::unique_ptr<http::Server> server_;

    // Helper function to create a simple handler for testing
    static void dummy_handler(const http::Request& req, http::Response& res) {
        res.setContent<http::ContentType::PLAIN_TEXT>("Test Response for " + req.path());
    }

    // SetUp runs before EVERY test in this fixture
    void SetUp() override {
        // Instantiate the server object before each test runs
        server_ = std::make_unique<http::Server>(test_host, test_port);
    }
};

#endif