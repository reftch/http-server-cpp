
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

    auto handler = [](const std::string& path, const auto&, const auto&) { return "Test " + path; };

    r.register_handler(method, path, handler);

    http::request_handler out_handler;
    std::unordered_map<std::string, std::string> out_params;
    std::unordered_map<std::string, std::string> query;

    ASSERT_FALSE(r.match("POST", path, &out_handler, &out_params, &query));
    ASSERT_FALSE(r.match("PATCH", path, &out_handler, &out_params, &query));
    ASSERT_FALSE(r.match("DELETE", path, &out_handler, &out_params, &query));

    ASSERT_TRUE(r.match(method, path, &out_handler, &out_params, &query));
    EXPECT_EQ("Test " + path, out_handler(path, out_params, query));
}

TEST_F(ServerTestFixture, RegisterPostHandlerSucceeds) {
    // Arrange: Define the handler to be registered
    http::router r;
    std::string path = "/api/v1/users";
    std::string method = "POST";

    // http::request_handler handler;
    std::unordered_map<std::string, std::string> params;

    auto handler = [](const std::string& path, const auto&, const auto&) { return "Test " + path; };

    r.register_handler(method, path, handler);

    http::request_handler out_handler;
    std::unordered_map<std::string, std::string> out_params;
    std::unordered_map<std::string, std::string> query;

    ASSERT_TRUE(r.match(method, path, &out_handler, &out_params, &query));
    EXPECT_EQ("Test " + path, out_handler(path, out_params, query));
}

TEST_F(ServerTestFixture, RegisterHandlerWithPathParameter) {
    // Arrange: Define the handler to be registered
    http::router r;
    std::string path = "/api/v1/users/:id";
    std::string method = "GET";

    // http::request_handler handler;
    std::unordered_map<std::string, std::string> params;
    std::unordered_map<std::string, std::string> query;

    auto handler = [](const std::string& path, const auto&, const auto&) { return "Test " + path; };

    r.register_handler(method, path, handler);

    http::request_handler out_handler;
    std::unordered_map<std::string, std::string> out_params;

    ASSERT_TRUE(r.match(method, path, &out_handler, &out_params, &query));
    EXPECT_EQ("Test " + path, out_handler(path, out_params, query));
    EXPECT_EQ(0, params.size());
}

TEST_F(ServerTestFixture, MatchQueryParametersWithSpaces) {
    http::router r;
    auto handler = [](const std::string&, const auto&, const auto&) { return "Test"; };
    r.register_handler("GET", "/search", handler);

    http::request_handler out_handler;
    std::unordered_map<std::string, std::string> out_params;
    std::unordered_map<std::string, std::string> query;

    ASSERT_TRUE(r.match("GET", "/search?query=hello world&filter=category 1", &out_handler, &out_params, &query));
    EXPECT_EQ(0, out_params.size());
    EXPECT_EQ(2, query.size());
    EXPECT_EQ("hello world", query["query"]);
    EXPECT_EQ("category 1", query["filter"]);
}

TEST_F(ServerTestFixture, MatchWithSingleQueryParameter) {
    http::router r;
    auto handler = [](const std::string&, const auto&, const auto&) { return "Test"; };
    r.register_handler("GET", "/users", handler);

    http::request_handler out_handler;
    std::unordered_map<std::string, std::string> out_params;
    std::unordered_map<std::string, std::string> query;

    ASSERT_TRUE(r.match("GET", "/users?name=John", &out_handler, &out_params, &query));
    EXPECT_EQ(0, out_params.size());
    EXPECT_EQ(1, query.size());
    EXPECT_EQ("John", query["name"]);
}

TEST_F(ServerTestFixture, MatchWithMultipleQueryParameters) {
    http::router r;
    auto handler = [](const std::string&, const auto&, const auto&) { return "Test"; };
    r.register_handler("GET", "/users", handler);

    http::request_handler out_handler;
    std::unordered_map<std::string, std::string> out_params;
    std::unordered_map<std::string, std::string> query;

    ASSERT_TRUE(r.match("GET", "/users?name=John&age=30&city=NYC", &out_handler, &out_params, &query));
    EXPECT_EQ(0, out_params.size());
    EXPECT_EQ(3, query.size());
    EXPECT_EQ("John", query["name"]);
    EXPECT_EQ("30", query["age"]);
    EXPECT_EQ("NYC", query["city"]);
}

TEST_F(ServerTestFixture, MatchWithPathParametersAndQueryParameters) {
    http::router r;
    auto handler = [](const std::string&, const auto&, const auto&) { return "Test"; };
    r.register_handler("GET", "/users/:id", handler);

    http::request_handler out_handler;
    std::unordered_map<std::string, std::string> out_params;
    std::unordered_map<std::string, std::string> query;

    ASSERT_TRUE(r.match("GET", "/users/123?format=json&include=posts", &out_handler, &out_params, &query));
    EXPECT_EQ(1, out_params.size());
    EXPECT_EQ("123", out_params["id"]);
    EXPECT_EQ(2, query.size());
    EXPECT_EQ("json", query["format"]);
    EXPECT_EQ("posts", query["include"]);
}

TEST_F(ServerTestFixture, MatchWithEmptyQueryParameters) {
    http::router r;
    auto handler = [](const std::string&, const auto&, const auto&) { return "Test"; };
    r.register_handler("GET", "/users", handler);

    http::request_handler out_handler;
    std::unordered_map<std::string, std::string> out_params;
    std::unordered_map<std::string, std::string> query;

    ASSERT_TRUE(r.match("GET", "/users?", &out_handler, &out_params, &query));
    EXPECT_EQ(0, out_params.size());
    EXPECT_EQ(0, query.size());
}

TEST_F(ServerTestFixture, MatchWithQueryParameterWithoutValue) {
    http::router r;
    auto handler = [](const std::string&, const auto&, const auto&) { return "Test"; };
    r.register_handler("GET", "/users", handler);

    http::request_handler out_handler;
    std::unordered_map<std::string, std::string> out_params;
    std::unordered_map<std::string, std::string> query;

    ASSERT_TRUE(r.match("GET", "/users?name", &out_handler, &out_params, &query));
    EXPECT_EQ(0, out_params.size());
    EXPECT_EQ(0, query.size());
}

TEST_F(ServerTestFixture, MatchWithSpecialCharactersInQueryParameters) {
    http::router r;
    auto handler = [](const std::string&, const auto&, const auto&) { return "Test"; };
    r.register_handler("GET", "/search", handler);

    http::request_handler out_handler;
    std::unordered_map<std::string, std::string> out_params;
    std::unordered_map<std::string, std::string> query;

    ASSERT_TRUE(r.match("GET", "/search?q=hello%20world&category=tech%2Bdev", &out_handler, &out_params, &query));
    EXPECT_EQ(0, out_params.size());
    EXPECT_EQ(2, query.size());
    EXPECT_EQ("hello%20world", query["q"]);
    EXPECT_EQ("tech%2Bdev", query["category"]);
}

TEST_F(ServerTestFixture, MatchWithRepeatedQueryParameterNames) {
    http::router r;
    auto handler = [](const std::string&, const auto&, const auto&) { return "Test"; };
    r.register_handler("GET", "/users", handler);

    http::request_handler out_handler;
    std::unordered_map<std::string, std::string> out_params;
    std::unordered_map<std::string, std::string> query;

    // Note: This tests how the parser handles repeated parameter names
    // The last value should overwrite previous ones
    ASSERT_TRUE(r.match("GET", "/users?filter=active&filter=verified", &out_handler, &out_params, &query));
    EXPECT_EQ(0, out_params.size());
    EXPECT_EQ(1, query.size());
    EXPECT_EQ("verified", query["filter"]);  // Last value wins
}

TEST_F(ServerTestFixture, MatchWithNoQueryParameters) {
    http::router r;
    auto handler = [](const std::string&, const auto&, const auto&) { return "Test"; };
    r.register_handler("GET", "/users/:id", handler);

    http::request_handler out_handler;
    std::unordered_map<std::string, std::string> out_params;
    std::unordered_map<std::string, std::string> query;

    ASSERT_TRUE(r.match("GET", "/users/123", &out_handler, &out_params, &query));
    EXPECT_EQ(1, out_params.size());
    EXPECT_EQ("123", out_params["id"]);
    EXPECT_EQ(0, query.size());
}