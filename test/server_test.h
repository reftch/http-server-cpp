#ifndef SERVER_TEST_HPP
#define SERVER_TEST_HPP

#include <thread>

#include "gtest/gtest.h"
#include "server.h"

// Define a fixture class to hold setup logic and shared objects
// class ServerTestFixture : public ::testing::Test {
//    protected:
//     // Define standard test parameters
//     // const std::string test_host = "127.0.0.1";
//     const std::string test_host = "0.0.0.0";
//     const int test_port = 8080;

//     // The object under test, managed by smart pointer
//     std::unique_ptr<http::Server> server_;

//     // Helper function to create a simple handler for testing
//     static void dummy_handler(const http::Request& req, http::Response& res) {
//         res.setContent<http::ContentType::PLAIN_TEXT>("Test Response for " + req.path());
//     }

//     // SetUp runs before EVERY test in this fixture
//     void SetUp() override {
//         // Instantiate the server object before each test runs
//         server_ = std::make_unique<http::Server>(test_host, test_port);
//     }
// };

class ServerTestFixture : public ::testing::Test {
   protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }

    static void startServer(std::unique_ptr<http::Server>& server) { server->start(); }

    static bool waitForServerStart(std::unique_ptr<http::Server>& server, int timeout_ms = 5000) {
        auto start_time = std::chrono::steady_clock::now();
        const auto timeout = std::chrono::milliseconds(timeout_ms);

        while (!server->is_running()) {
            if (std::chrono::steady_clock::now() - start_time > timeout) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }

    static void stopServer(std::unique_ptr<http::Server>& server, std::thread& server_thread) {
        server->stop();
        if (server_thread.joinable()) {
            server_thread.join();
        }
    }
};

#endif