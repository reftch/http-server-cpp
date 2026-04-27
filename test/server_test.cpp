#include "server.hpp"  // Include the header for the class we are testing

#include <memory>
#include <regex>
#include <string>
#include <unordered_map>

#include "gtest/gtest.h"

using namespace http::server;

// ====================================================================
// Test Fixture Setup
// ====================================================================

// Define a fixture class to hold setup logic and shared objects
class ServerTestFixture : public ::testing::Test {
   protected:
    // Define standard test parameters
    const std::string test_host = "127.0.0.1";
    const std::string test_port = "8080";

    // The object under test, managed by smart pointer
    std::unique_ptr<server> server_;

    // Helper function to create a simple handler for testing
    static response_body dummy_handler(const std::string& path,
                                       const std::unordered_map<std::string, std::string>& params) {
        return "Test Response for " + path;
    }

    // SetUp runs before EVERY test in this fixture
    void SetUp() override {
        // Instantiate the server object before each test runs
        server_ = std::make_unique<server>(test_host, test_port);
    }
};

// Test Suite for checking initial state and configuration
TEST_F(ServerTestFixture, ConstructorSetsState) {
    // Assert that the server object was initialized correctly
    EXPECT_EQ(test_host, server_->get_host());
    EXPECT_EQ(test_port, server_->get_port());
    EXPECT_FALSE(server_->is_running());
}

// Test Suite for testing route registration
TEST_F(ServerTestFixture, RegisterHandlerSucceeds) {
    // Arrange: Define the handler to be registered
    auto handler = ServerTestFixture::dummy_handler;
    std::string path = "/api/status";
    std::string method = "GET";

    // Act: Register the handler
    int result = server_->register_handler(method, path, handler);

    // Assert: Check the return code and internal state
    EXPECT_EQ(0, result);  // Expect success
    ASSERT_FALSE(server_->get_routes().empty());

    // Check the details of the registered route
    const auto& routes = server_->get_routes();
    EXPECT_EQ(method, routes[0].method);
    EXPECT_EQ(path, routes[0].pattern);
    ASSERT_TRUE(routes[0].param_names.empty());
}

void stop_server_thread(std::unique_ptr<server>& server_ptr) {
    std::cout << "thread reading server host: " << server_ptr->get_host() << std::endl;
    // Pause the execution of this specific thread for the 1 second
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // check running state
    EXPECT_TRUE(server_ptr->is_running());
    server_ptr->stop();
}

// Test Suite for testing server control flow
TEST_F(ServerTestFixture, StartAndStopControlsState) {
    std::thread t(stop_server_thread, std::ref(server_));

    // start the server
    int start_result = server_->start();
    EXPECT_EQ(0, start_result);  // expect success

    // The main thread will pause here until the worker_thread completes its execution
    t.join();

    // check stopped state
    EXPECT_FALSE(server_->is_running());
}

// ====================================================================
// Main Entry Point
// ====================================================================

int main(int argc, char** argv) {
    // This line initializes the Google Test framework and runs all tests
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}