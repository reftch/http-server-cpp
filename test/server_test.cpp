#include "server.hpp"  // Include the header for the class we are testing

#include <memory>
#include <regex>
#include <string>
#include <unordered_map>

#include "server_test.hpp"

using namespace http;

// ====================================================================
// Test Fixture Setup
// ====================================================================

// Test Suite for checking initial state and configuration
TEST_F(ServerTestFixture, ConstructorSetsState) {
    // Assert that the server object was initialized correctly
    EXPECT_EQ(test_host, server_->get_host());
    EXPECT_EQ(test_port, server_->get_port());
    EXPECT_FALSE(server_->running());
}

// Test Suite for testing route registration
// TEST_F(ServerTestFixture, RegisterHandlerSucceeds) {
//     // Arrange: Define the handler to be registered
//     auto handler = ServerTestFixture::dummy_handler;
//     std::string path = "/api/status";
//     std::string method = "GET";

//     // Act: Register the handler
//     int result = server_->path(method, path, handler);

//     // Assert: Check the return code and internal state
//     EXPECT_EQ(0, result);  // Expect success
//     // ASSERT_FALSE(server_->get_routes().empty());

//     // Check the details of the registered route
//     const auto& routes = server_->get_routes();
//     EXPECT_EQ(method, routes[0].method);
//     EXPECT_EQ(path, routes[0].pattern);
//     ASSERT_TRUE(routes[0].param_names.empty());
// }

// TEST_F(ServerTestFixture, RegisterPostHandlerSucceeds) {
//     // Arrange: Define the handler to be registered
//     auto handler = ServerTestFixture::dummy_handler;
//     std::string path = "/api/user";
//     std::string method = "POST";

//     // Act: Register the handler
//     int result = server_->register_handler(method, path, handler);

//     // Assert: Check the return code and internal state
//     EXPECT_EQ(0, result);  // Expect success
//     ASSERT_FALSE(server_->get_routes().empty());

//     // Check the details of the registered route
//     const auto& routes = server_->get_routes();
//     EXPECT_EQ(method, routes[0].method);
// }

// TEST_F(ServerTestFixture, RegisterHandlerWithPathParameter) {
//     // Arrange: Define the handler to be registered
//     auto handler = ServerTestFixture::dummy_handler;
//     std::string path = "/api/v1/users/:id";
//     std::string method = "GET";

//     // Act: Register the handler
//     int result = server_->register_handler(method, path, handler);

//     // Assert: Check the return code and internal state
//     EXPECT_EQ(0, result);  // Expect success
//     ASSERT_FALSE(server_->get_routes().empty());

//     // Check the details of the registered route
//     const auto& routes = server_->get_routes();
//     EXPECT_EQ(method, routes[0].method);
//     EXPECT_EQ(path, routes[0].pattern);

//     ASSERT_FALSE(routes[0].param_names.empty());
//     EXPECT_EQ(1, routes[0].param_names.size());
//     EXPECT_EQ("id", routes[0].param_names[0]);
// }

// TEST_F(ServerTestFixture, RegisterHandlerWithPathMultipleParameter) {
//     // Arrange: Define the handler to be registered
//     auto handler = ServerTestFixture::dummy_handler;
//     std::string path = "/api/v1/users/:id/:age";
//     std::string method = "GET";

//     // Act: Register the handler
//     int result = server_->register_handler(method, path, handler);

//     // Assert: Check the return code and internal state
//     EXPECT_EQ(0, result);  // Expect success
//     ASSERT_FALSE(server_->get_routes().empty());

//     // Check the details of the registered route
//     const auto& routes = server_->get_routes();
//     EXPECT_EQ(method, routes[0].method);
//     EXPECT_EQ(path, routes[0].pattern);

//     ASSERT_FALSE(routes[0].param_names.empty());
//     EXPECT_EQ(2, routes[0].param_names.size());
//     EXPECT_EQ("id", routes[0].param_names[0]);
//     EXPECT_EQ("age", routes[0].param_names[1]);
// }

void stop_server_thread(std::unique_ptr<server>& server_ptr) {
    std::cout << "thread reading server host: " << server_ptr->get_host() << std::endl;
    // Pause the execution of this specific thread for the 1 second
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // check running state
    EXPECT_TRUE(server_ptr->running());
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
    EXPECT_FALSE(server_->running());
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