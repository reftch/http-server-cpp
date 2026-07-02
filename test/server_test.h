#ifndef SERVER_TEST_HPP
#define SERVER_TEST_HPP

#include <mutex>
#include <thread>

#include "gtest/gtest.h"
#include "server.h"

// Define a fixture class to hold setup logic and shared objects
static std::mutex server_mutex_;

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

        // std::lock_guard<std::mutex> lock(server_mutex_);
        std::lock_guard<std::mutex> lock(server_mutex_);
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

    // static bool waitForServerStart(std::unique_ptr<http::Server>& server, int timeout_ms = 5000) {
    //     auto start_time = std::chrono::steady_clock::now();
    //     const auto timeout = std::chrono::milliseconds(timeout_ms);

    //     while (true) {
    //         bool is_running = false;

    //         // Thread-safe access to server state
    //         {
    //             std::lock_guard<std::mutex> lock(server_mutex_);
    //             if (server) {  // Safety check
    //                 is_running = server->is_running();
    //             }
    //         }

    //         if (is_running) {
    //             return true;
    //         }

    //         if (std::chrono::steady_clock::now() - start_time > timeout) {
    //             return false;
    //         }
    //         std::this_thread::sleep_for(std::chrono::milliseconds(10));
    //     }
    // }

    // static void stopServer(std::unique_ptr<http::Server>& server, std::thread& server_thread) {
    //     {
    //         std::lock_guard<std::mutex> lock(server_mutex_);
    //         if (server) {
    //             server->stop();
    //         }
    //     }
    //     if (server_thread.joinable()) {
    //         server_thread.join();
    //     }
    // }
};

#endif