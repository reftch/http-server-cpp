#ifndef SERVER_TEST_HPP
#define SERVER_TEST_HPP

#include <thread>

#include "gtest/gtest.h"
#include "server.h"

// Define a fixture class to hold setup logic and shared objects
class ServerTestFixture : public ::testing::Test {
   protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }

    static void startServer(std::unique_ptr<http::Server>& server) {
        if (server) {
            server->start();
        }
    }

    bool waitForServerStart(std::unique_ptr<http::Server>& server, int timeout_ms = 5000) {
        if (!server) return false;

        auto start_time = std::chrono::steady_clock::now();
        const auto timeout = std::chrono::milliseconds(timeout_ms);

        // Lock the mutex to protect access to server state
        while (!server->is_running()) {
            if (std::chrono::steady_clock::now() - start_time > timeout) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }

    void stopServer(std::unique_ptr<http::Server>& server, std::thread& server_thread) {
        if (server) {
            server->stop();
        }

        if (server_thread.joinable()) {
            server_thread.join();
        }
    }
};

#endif