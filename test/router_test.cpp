
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

    auto handler = [](const http::request::context& ctx) { return "Test " + ctx.path; };

    r.register_handler(method, path, handler);

    http::request_handler out_handler;

    http::request::context ctx = {};
    ctx.method = method;
    ctx.path = path;

    ASSERT_TRUE(r.match(&ctx, &out_handler));
    EXPECT_EQ("Test " + path, out_handler(ctx));
}

TEST_F(ServerTestFixture, RegisterPostHandlerSucceeds) {
    // Arrange
    http::router r;
    std::string path = "/api/v1/users";
    std::string method = "POST";

    auto handler = [](const http::request::context& ctx) { return "Test " + ctx.path; };
    r.register_handler(method, path, handler);

    // Act
    http::request::context ctx = {};
    ctx.method = method;
    ctx.path = path;

    http::request_handler out_handler;
    bool matched = r.match(&ctx, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ("Test " + path, out_handler(ctx));
}

TEST_F(ServerTestFixture, RegisterHandlerWithPathParameter) {
    // Arrange
    http::router r;
    std::string path = "/api/v1/users/:id";
    std::string method = "GET";

    auto handler = [](const http::request::context& ctx) { return "Test " + ctx.path; };
    r.register_handler(method, path, handler);

    // Act
    http::request::context ctx = {};
    ctx.method = method;
    ctx.path = "/api/v1/users/123";

    http::request_handler out_handler;
    bool matched = r.match(&ctx, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ("Test /api/v1/users/123", out_handler(ctx));
    EXPECT_EQ("123", ctx.params["id"]);
}

TEST_F(ServerTestFixture, MatchQueryParametersWithSpaces) {
    // Arrange
    http::router r;
    auto handler = [](const http::request::context&) { return "Test"; };
    r.register_handler("GET", "/search", handler);

    // Act
    http::request::context ctx = {};
    ctx.method = "GET";
    ctx.path = "/search?query=hello world&filter=category 1";

    http::request_handler out_handler;
    bool matched = r.match(&ctx, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(2, ctx.query.size());
    EXPECT_EQ("hello world", ctx.query["query"]);
    EXPECT_EQ("category 1", ctx.query["filter"]);
}

TEST_F(ServerTestFixture, MatchWithSingleQueryParameter) {
    // Arrange
    http::router r;
    auto handler = [](const http::request::context&) { return "Test"; };
    r.register_handler("GET", "/users", handler);

    // Act
    http::request::context ctx = {};
    ctx.method = "GET";
    ctx.path = "/users?name=John";

    http::request_handler out_handler;
    bool matched = r.match(&ctx, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(1, ctx.query.size());
    EXPECT_EQ("John", ctx.query["name"]);
}

TEST_F(ServerTestFixture, MatchWithMultipleQueryParameters) {
    // Arrange
    http::router r;
    auto handler = [](const http::request::context&) { return "Test"; };
    r.register_handler("GET", "/users", handler);

    // Act
    http::request::context ctx = {};
    ctx.method = "GET";
    ctx.path = "/users?name=John&age=30&city=NYC";

    http::request_handler out_handler;
    bool matched = r.match(&ctx, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(3, ctx.query.size());
    EXPECT_EQ("John", ctx.query["name"]);
    EXPECT_EQ("30", ctx.query["age"]);
    EXPECT_EQ("NYC", ctx.query["city"]);
}

TEST_F(ServerTestFixture, MatchWithPathParametersAndQueryParameters) {
    // Arrange
    http::router r;
    auto handler = [](const http::request::context&) { return "Test"; };
    r.register_handler("GET", "/users/:id", handler);

    // Act
    http::request::context ctx = {};
    ctx.method = "GET";
    ctx.path = "/users/123?format=json&include=posts";

    http::request_handler out_handler;
    bool matched = r.match(&ctx, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(1, ctx.params.size());
    EXPECT_EQ("123", ctx.params["id"]);
    EXPECT_EQ(2, ctx.query.size());
    EXPECT_EQ("json", ctx.query["format"]);
    EXPECT_EQ("posts", ctx.query["include"]);
}

TEST_F(ServerTestFixture, MatchWithEmptyQueryParameters) {
    // Arrange
    http::router r;
    auto handler = [](const http::request::context&) { return "Test"; };
    r.register_handler("GET", "/users", handler);

    // Act
    http::request::context ctx = {};
    ctx.method = "GET";
    ctx.path = "/users?";

    http::request_handler out_handler;
    bool matched = r.match(&ctx, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(0, ctx.query.size());
}

TEST_F(ServerTestFixture, MatchWithQueryParameterWithoutValue) {
    // Arrange
    http::router r;
    auto handler = [](const http::request::context&) { return "Test"; };
    r.register_handler("GET", "/users", handler);

    // Act
    http::request::context ctx = {};
    ctx.method = "GET";
    ctx.path = "/users?name";

    http::request_handler out_handler;
    bool matched = r.match(&ctx, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(0, ctx.query.size());  // Empty params are ignored
}

TEST_F(ServerTestFixture, MatchWithSpecialCharactersInQueryParameters) {
    // Arrange
    http::router r;
    auto handler = [](const http::request::context&) { return "Test"; };
    r.register_handler("GET", "/search", handler);

    // Act
    http::request::context ctx = {};
    ctx.method = "GET";
    ctx.path = "/search?q=hello%20world&category=tech%2Bdev";

    http::request_handler out_handler;
    bool matched = r.match(&ctx, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(2, ctx.query.size());
    EXPECT_EQ("hello%20world", ctx.query["q"]);
    EXPECT_EQ("tech%2Bdev", ctx.query["category"]);
}

TEST_F(ServerTestFixture, MatchWithRepeatedQueryParameterNames) {
    // Arrange
    http::router r;
    auto handler = [](const http::request::context&) { return "Test"; };
    r.register_handler("GET", "/users", handler);

    // Act
    http::request::context ctx = {};
    ctx.method = "GET";
    ctx.path = "/users?filter=active&filter=verified";

    http::request_handler out_handler;
    bool matched = r.match(&ctx, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(1, ctx.query.size());
    EXPECT_EQ("verified", ctx.query["filter"]);  // Last value wins
}

TEST_F(ServerTestFixture, MatchWithNoQueryParameters) {
    // Arrange
    http::router r;
    auto handler = [](const http::request::context&) { return "Test"; };
    r.register_handler("GET", "/users/:id", handler);

    // Act
    http::request::context ctx = {};
    ctx.method = "GET";
    ctx.path = "/users/123";

    http::request_handler out_handler;
    bool matched = r.match(&ctx, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(1, ctx.params.size());
    EXPECT_EQ("123", ctx.params["id"]);
    EXPECT_EQ(0, ctx.query.size());
}
