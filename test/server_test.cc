#include "server.h"

#include <memory>
#include <regex>
#include <string>
#include <unordered_map>

#include "server_test.h"

using namespace http;

// ====================================================================
// Test Fixture Setup
// ====================================================================

// Test Suite for checking initial state and configuration
TEST_F(ServerTestFixture, ConstructorSetsState) {
    // Assert that the server object was initialized correctly
    EXPECT_EQ(test_host, server_->get_host());
    EXPECT_EQ(test_port, server_->get_port());
    EXPECT_FALSE(server_->is_running());
}

void stop_server_thread(std::unique_ptr<Server>& server_ptr) {
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