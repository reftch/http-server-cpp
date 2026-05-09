
#include "server.hpp"
#include "server_test.hpp"

// Test Suite for testing route registration
TEST_F(ServerTestFixture, RegisterHandlerSucceeds) {
    // Arrange: Define the handler to be registered
    http::router r;
    std::string path = "/api/status";
    std::string method = "GET";

    // http::request_handler handler;
    std::unordered_map<std::string, std::string> params;

    auto handler = [](const std::string& path, const auto&) { return "Test " + path; };

    r.register_handler(method, path, handler);

    http::request_handler out_handler;
    std::unordered_map<std::string, std::string> out_params;

    ASSERT_FALSE(r.match("POST", path, &out_handler, &out_params));
    ASSERT_FALSE(r.match("PATCH", path, &out_handler, &out_params));
    ASSERT_FALSE(r.match("DELETE", path, &out_handler, &out_params));

    ASSERT_TRUE(r.match(method, path, &out_handler, &out_params));
    EXPECT_EQ("Test " + path, out_handler(path, out_params));
}

TEST_F(ServerTestFixture, RegisterPostHandlerSucceeds) {
    // Arrange: Define the handler to be registered
    http::router r;
    std::string path = "/api/v1/users";
    std::string method = "POST";

    // http::request_handler handler;
    std::unordered_map<std::string, std::string> params;

    auto handler = [](const std::string& path, const auto&) { return "Test " + path; };

    r.register_handler(method, path, handler);

    http::request_handler out_handler;
    std::unordered_map<std::string, std::string> out_params;

    ASSERT_TRUE(r.match(method, path, &out_handler, &out_params));
    EXPECT_EQ("Test " + path, out_handler(path, out_params));
}

TEST_F(ServerTestFixture, RegisterHandlerWithPathParameter) {
    // Arrange: Define the handler to be registered
    http::router r;
    std::string path = "/api/v1/users/:id";
    std::string method = "GET";

    // http::request_handler handler;
    std::unordered_map<std::string, std::string> params;

    auto handler = [](const std::string& path, const auto&) { return "Test " + path; };

    r.register_handler(method, path, handler);

    http::request_handler out_handler;
    std::unordered_map<std::string, std::string> out_params;

    ASSERT_TRUE(r.match(method, path, &out_handler, &out_params));
    EXPECT_EQ("Test " + path, out_handler(path, out_params));
    EXPECT_EQ(0, params.size());
}