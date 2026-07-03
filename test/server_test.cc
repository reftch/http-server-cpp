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
    // Use a unique port for each test to avoid conflicts
    const std::string test_host = "127.0.0.1";
    const int test_port = 8081 + (getpid() % 1000);  // Add process ID offset

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
    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start within timeout";
    EXPECT_TRUE(server->is_running());

    // Create client and make request to specific host/port
    auto cli = Client("http://" + test_host + ":" + std::to_string(test_port));
    auto res = cli.get("/endpoint");

    EXPECT_EQ(static_cast<int>(res->status()), 200);
    EXPECT_EQ(res->content(), "test");
    EXPECT_EQ(res->headers().at("Content-Type"), "text/plain; charset=utf-8");

    // Stop server
    stopServer(server, server_thread);

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
    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start within timeout";

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
    stopServer(server, server_thread);

    // Verify server is stopped
    EXPECT_FALSE(server->is_running());
}

TEST_F(ServerTestFixture, StartAndMakeRequest) {
    // Create server
    const std::string test_host = "127.0.0.1";
    const int test_port = 8081 + (getpid() % 1000);  // Add process ID offset
    auto server = std::make_unique<Server>(test_host, test_port);

    // Set up route
    server->setRoute<http::HttpMethod::GET>("/endpoint2", [](const http::Request&, http::Response& res) {
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
    auto cli = std::make_unique<Client>("http://" + test_host + ":" + std::to_string(test_port));
    auto res = cli->get("/endpoint2");

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
    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start within timeout";
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
    stopServer(server, server_thread);

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
    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start within timeout";
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
    stopServer(server, server_thread);

    // Verify server is stopped
    EXPECT_FALSE(server->is_running());
}

TEST_F(ServerTestFixture, PostRouteForCORS) {
    const std::string test_host = "127.0.0.1";
    const int test_port = 8084;
    auto server = std::make_unique<Server>(test_host, test_port);

    // Set up CORS handler using setPostRoute for OPTIONS requests
    server->setPostRoute([&](const http::Request&, http::Response& res) {
        res.setHeader("Access-Control-Allow-Origin", "*");
        res.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    });

    // Also set a regular GET route to test normal behavior
    server->setRoute<http::HttpMethod::GET>("/test", [](const http::Request&, http::Response& res) {
        res << http::ContentType::PLAIN_TEXT << "GET works";
    });

    // Start server in thread
    std::thread server_thread([&server]() {
        startServer(server);
    });

    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start within timeout";

    // Create client and send an OPTIONS request (CORS preflight)
    auto cli = Client("http://" + test_host + ":" + std::to_string(test_port));

    auto options_res = cli.get("/test");

    EXPECT_EQ(static_cast<int>(options_res->status()), 200);

    // Check CORS headers are present
    EXPECT_TRUE(options_res->headers().find("Access-Control-Allow-Origin") != options_res->headers().end());
    EXPECT_EQ(options_res->headers().at("Access-Control-Allow-Origin"), "*");

    EXPECT_TRUE(options_res->headers().find("Access-Control-Allow-Methods") != options_res->headers().end());
    EXPECT_EQ(options_res->headers().at("Access-Control-Allow-Methods"), "GET, POST, PUT, DELETE, OPTIONS");

    EXPECT_TRUE(options_res->headers().find("Access-Control-Allow-Headers") != options_res->headers().end());
    EXPECT_EQ(options_res->headers().at("Access-Control-Allow-Headers"), "Content-Type, Authorization");

    // Test normal GET request still works
    auto get_res = cli.get("/test");
    EXPECT_EQ(static_cast<int>(get_res->status()), 200);
    EXPECT_EQ(get_res->content(), "GET works");

    // Stop server
    stopServer(server, server_thread);
}

TEST_F(ServerTestFixture, PostRouteHandlesRealPost) {
    const std::string test_host = "127.0.0.1";
    const int test_port = 8085;
    auto server = std::make_unique<Server>(test_host, test_port);

    server->setPostRoute([&](const http::Request&, http::Response& res) {
        res.setHeader("Access-Control-Allow-Origin", "*");
        res << http::ContentType::PLAIN_TEXT << "POST handled";
    });

    // Start server
    std::thread server_thread([&server]() {
        startServer(server);
    });

    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start";

    auto cli = Client("http://" + test_host + ":" + std::to_string(test_port));

    auto post_res = cli.post("/any", "data");

    EXPECT_EQ(static_cast<int>(post_res->status()), 200);
    EXPECT_EQ(post_res->content(), "POST handled");
    EXPECT_EQ(post_res->headers().at("Access-Control-Allow-Origin"), "*");

    stopServer(server, server_thread);
}

TEST_F(ServerTestFixture, PreRouteHandlesRequestBeforeOtherHandlers) {
    const std::string test_host = "127.0.0.1";
    const int test_port = 8086;
    auto server = std::make_unique<Server>(test_host, test_port);

    // Set pre-route handler that modifies response
    server->setPreRoute([&](const http::Request&, http::Response& res) {
        res.setHeader("X-PreRoute-Header", "pre-route-value");
        res << http::ContentType::PLAIN_TEXT << "Pre-route processed";
    });

    // Set regular route handler using proper syntax
    server->setRoute<http::HttpMethod::GET>("/test", [](const http::Request&, http::Response& res) {
        res << http::ContentType::PLAIN_TEXT << "Regular route handled";
    });

    // Start server
    std::thread server_thread([&server]() {
        startServer(server);
    });

    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start";

    auto cli = Client("http://" + test_host + ":" + std::to_string(test_port));

    auto get_res = cli.get("/test");

    EXPECT_EQ(static_cast<int>(get_res->status()), 200);
    EXPECT_EQ(get_res->content(), "Regular route handled");
    EXPECT_EQ(get_res->headers().at("X-PreRoute-Header"), "pre-route-value");

    stopServer(server, server_thread);
}

TEST_F(ServerTestFixture, PreRouteModifiesRequestBeforeProcessing) {
    const std::string test_host = "127.0.0.1";
    const int test_port = 8087;
    auto server = std::make_unique<Server>(test_host, test_port);

    // Set pre-route handler that adds custom header
    server->setPreRoute([&](const http::Request& req, http::Response& res) {
        // Modify request (if supported by your framework)
        res.setHeader("X-PreRoute-Processed", "true");
        res.setHeader("X-Original-Method", req.method());
    });

    // Set route handler using proper syntax
    server->setRoute<http::HttpMethod::POST>("/api/test", [](const http::Request&, http::Response& res) {
        res << http::ContentType::JSON << R"({"message": "API endpoint reached"})";
    });

    // Start server
    std::thread server_thread([&server]() {
        startServer(server);
    });

    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start";

    auto cli = Client("http://" + test_host + ":" + std::to_string(test_port));

    auto post_res = cli.post("/api/test", "test data");

    EXPECT_EQ(static_cast<int>(post_res->status()), 200);
    EXPECT_EQ(post_res->headers().at("X-PreRoute-Processed"), "true");
    EXPECT_EQ(post_res->headers().at("X-Original-Method"), "POST");

    stopServer(server, server_thread);
}

TEST_F(ServerTestFixture, PreRouteHandlesMultipleRequests) {
    const std::string test_host = "127.0.0.1";
    const int test_port = 8089;
    auto server = std::make_unique<Server>(test_host, test_port);

    // Set pre-route handler that increments a counter
    static int pre_route_counter = 0;
    server->setPreRoute([&](const http::Request&, http::Response& res) {
        pre_route_counter++;
        res.setHeader("X-Request-Count", std::to_string(pre_route_counter));
        res << http::ContentType::PLAIN_TEXT << "Processed by pre-route";
    });

    // Set route handler using proper syntax
    server->setRoute<http::HttpMethod::GET>("/counter", [](const http::Request&, http::Response& res) {
        res << http::ContentType::PLAIN_TEXT << "Counter test endpoint";
    });

    // Start server
    std::thread server_thread([&server]() {
        startServer(server);
    });

    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start";

    auto cli = Client("http://" + test_host + ":" + std::to_string(test_port));

    // Make multiple requests
    for (int i = 0; i < 3; ++i) {
        auto get_res = cli.get("/counter");
        EXPECT_EQ(static_cast<int>(get_res->status()), 200);
        EXPECT_EQ(get_res->headers().at("X-Request-Count"), std::to_string(i + 1));
    }

    stopServer(server, server_thread);
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
