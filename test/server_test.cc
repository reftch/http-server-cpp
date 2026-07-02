#include "server.h"

#include <memory>
#include <string>
#include <thread>

#include "client.h"
#include "server_test.h"

using namespace http;

// ====================================================================
// Test Fixture Setup
// ====================================================================

TEST_F(ServerTestFixture, SetHostAndPort) {
    // Create server with host and port in constructor
    const std::string test_host = "127.0.0.1";
    const int test_port = 8081;

    auto server = std::make_unique<Server>(test_host, test_port);

    // Set up route
    server->setRoute<http::HttpMethod::GET>("/endpoint", [](const http::Request&, http::Response& res) {
        res << http::ContentType::PLAIN_TEXT << "test";
    });

    // Start server in separate thread
    std::thread server_thread([&server]() {
        server->start();
    });

    // Wait for server to start
    auto start_time = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(5);

    while (!server->is_running()) {
        if (std::chrono::steady_clock::now() - start_time > timeout) {
            FAIL() << "Server failed to start within timeout";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(server->is_running());

    // Create client and make request to specific host/port
    auto cli = Client("http://" + test_host + ":" + std::to_string(test_port));
    auto res = cli.get("/endpoint");

    EXPECT_EQ(static_cast<int>(res->status()), 200);
    EXPECT_EQ(res->content(), "test");
    EXPECT_EQ(res->headers().at("Content-Type"), "text/plain; charset=utf-8");

    // Stop server
    server->stop();
    server_thread.join();

    // Verify server is stopped
    EXPECT_FALSE(server->is_running());
}

TEST_F(ServerTestFixture, DefaultHostAndPort) {
    // Create server with default settings
    auto server = std::make_unique<Server>();

    // Set up route
    server->setRoute<http::HttpMethod::GET>("/endpoint", [](const http::Request&, http::Response& res) {
        res << http::ContentType::PLAIN_TEXT << "test";
    });

    // Start server in separate thread
    std::thread server_thread([&server]() {
        server->start();
    });

    // Wait for server to start
    auto start_time = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(5);

    while (!server->is_running()) {
        if (std::chrono::steady_clock::now() - start_time > timeout) {
            FAIL() << "Server failed to start within timeout";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(server->is_running());

    // Test default host and port (assuming defaults are 0.0.0.0:8080)
    const std::string default_host = "0.0.0.0";
    const int default_port = 8080;

    EXPECT_EQ(server->host(), default_host);
    EXPECT_EQ(server->port(), default_port);

    // Create client and make request
    auto cli = Client("http://" + default_host + ":" + std::to_string(default_port));
    auto res = cli.get("/endpoint");

    EXPECT_EQ(static_cast<int>(res->status()), 200);
    EXPECT_EQ(res->content(), "test");
    EXPECT_EQ(res->headers().at("Content-Type"), "text/plain; charset=utf-8");

    // Stop server
    server->stop();
    server_thread.join();

    // Verify server is stopped
    EXPECT_FALSE(server->is_running());
}

TEST_F(ServerTestFixture, StartAndMakeRequest) {
    // Create server
    auto server = std::make_unique<Server>();

    // Set up route
    server->setRoute<http::HttpMethod::GET>("/endpoint", [](const http::Request&, http::Response& res) {
        res << http::ContentType::PLAIN_TEXT << "test";
    });

    // Start server in separate thread
    std::thread server_thread([&server]() {
        startServer(server);
    });

    // Wait for server to start
    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start within timeout";

    EXPECT_TRUE(server->is_running());

    // Create client and make request
    auto cli = Client("http://0.0.0.0:8080");
    auto res = cli.get("/endpoint");

    EXPECT_EQ(static_cast<int>(res->status()), 200);
    EXPECT_EQ(res->content(), "test");
    EXPECT_EQ(res->headers().at("Content-Type"), "text/plain; charset=utf-8");

    // Stop server
    stopServer(server, server_thread);

    // Verify server is stopped
    EXPECT_FALSE(server->is_running());
}

TEST_F(ServerTestFixture, SetDefaultHeaders) {
    // Create server
    const std::string test_host = "127.0.0.1";
    const int test_port = 8082;
    auto server = std::make_unique<Server>(test_host, test_port);

    // Set default headers
    server->setDefaultHeaders({{"Connection", "keep-alive"}, {"X-Custom-Header", "test-value"}});

    // Set up route
    server->setRoute<http::HttpMethod::GET>("/header1", [](const http::Request&, http::Response& res) {
        res << http::ContentType::PLAIN_TEXT << "test";
    });

    // Start server in separate thread
    std::thread server_thread([&server]() {
        server->start();
    });

    // Wait for server to start
    auto start_time = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(5);

    while (!server->is_running()) {
        if (std::chrono::steady_clock::now() - start_time > timeout) {
            FAIL() << "Server failed to start within timeout";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(server->is_running());

    // Create client and make request
    auto cli = Client("http://0.0.0.0:8082");
    auto res = cli.get("/header1");

    EXPECT_EQ(static_cast<int>(res->status()), 200);
    EXPECT_EQ(res->content(), "test");

    // Check that default headers are present
    EXPECT_TRUE(res->headers().find("Connection") != res->headers().end());
    EXPECT_EQ(res->headers().at("Connection"), "keep-alive");
    EXPECT_TRUE(res->headers().find("X-Custom-Header") != res->headers().end());
    EXPECT_EQ(res->headers().at("X-Custom-Header"), "test-value");

    // Stop server
    server->stop();
    server_thread.join();

    // Verify server is stopped
    EXPECT_FALSE(server->is_running());
}

TEST_F(ServerTestFixture, SetDefaultHeadersEmpty) {
    // Create server
    const std::string test_host = "127.0.0.1";
    const int test_port = 8083;
    auto server = std::make_unique<Server>(test_host, test_port);

    // Set empty default headers
    server->setDefaultHeaders({});

    // Set up route
    server->setRoute<http::HttpMethod::GET>("/endpoint3", [](const http::Request&, http::Response& res) {
        res << http::ContentType::PLAIN_TEXT << "test";
    });

    // Start server in separate thread
    std::thread server_thread([&server]() {
        server->start();
    });

    // Wait for server to start
    auto start_time = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(5);

    while (!server->is_running()) {
        if (std::chrono::steady_clock::now() - start_time > timeout) {
            FAIL() << "Server failed to start within timeout";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(server->is_running());

    // Create client and make request
    auto cli = Client("http://0.0.0.0:8083");
    auto res = cli.get("/endpoint3");

    EXPECT_EQ(static_cast<int>(res->status()), 200);
    EXPECT_EQ(res->content(), "test");

    // Verify that no default headers are added (or at least don't interfere)
    // This test assumes that empty default headers don't add any extra headers
    // The exact behavior depends on your implementation

    // Stop server
    server->stop();
    server_thread.join();

    // Verify server is stopped
    EXPECT_FALSE(server->is_running());
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
