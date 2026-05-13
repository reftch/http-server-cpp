
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

    auto handler = [](const http::context& ctx) { return "Test " + ctx.request.path; };

    r.register_handler(method, path, handler);

    http::request_handler out_handler;

    http::request::context request = {};
    request.method = method;
    request.path = path;

    http::context ctx = {request};

    ASSERT_TRUE(r.match(&ctx, &out_handler));
    EXPECT_EQ("Test " + path, out_handler(ctx));
}

TEST_F(ServerTestFixture, RegisterPostHandlerSucceeds) {
    // Arrange: Define the handler to be registered
    http::router r;
    std::string path = "/api/v1/users";
    std::string method = "POST";

    auto handler = [](const http::context& ctx) { return "Test " + ctx.request.path; };

    r.register_handler(method, path, handler);

    http::request_handler out_handler;

    http::request::context request = {};
    request.method = method;
    request.path = path;

    http::context ctx = {request};

    ASSERT_TRUE(r.match(&ctx, &out_handler));
    EXPECT_EQ("Test " + path, out_handler(ctx));
}

TEST_F(ServerTestFixture, RegisterHandlerWithPathParameter) {
    // Arrange: Define the handler to be registered
    http::router r;
    std::string path = "/api/v1/users/:id";
    std::string method = "GET";

    auto handler = [](const http::context& ctx) { return "Test " + ctx.request.path; };

    r.register_handler(method, path, handler);

    http::request_handler out_handler;

    http::request::context request = {};
    request.method = method;
    request.path = path;

    http::context ctx = {request};

    ASSERT_TRUE(r.match(&ctx, &out_handler));
    EXPECT_EQ("Test " + path, out_handler(ctx));
}

TEST_F(ServerTestFixture, MatchQueryParametersWithSpaces) {
    http::router r;
    auto handler = [](const http::context&) { return "Test"; };
    r.register_handler("GET", "/search", handler);

    http::request::context request = {};
    request.method = "GET";
    request.path = "/search?query=hello world&filter=category 1";

    http::context ctx = {request};

    http::request_handler out_handler;
    ASSERT_TRUE(r.match(&ctx, &out_handler));
    EXPECT_EQ(2, ctx.request.query.size());
    EXPECT_EQ("hello world", ctx.request.query["query"]);
    EXPECT_EQ("category 1", ctx.request.query["filter"]);
}

TEST_F(ServerTestFixture, MatchWithSingleQueryParameter) {
    http::router r;
    auto handler = [](const http::context&) { return "Test"; };
    r.register_handler("GET", "/users", handler);

    http::request::context request = {};
    request.method = "GET";
    request.path = "/users?name=John";

    http::context ctx = {request};

    http::request_handler out_handler;
    ASSERT_TRUE(r.match(&ctx, &out_handler));
    EXPECT_EQ(1, ctx.request.query.size());
    EXPECT_EQ("John", ctx.request.query["name"]);
}

TEST_F(ServerTestFixture, MatchWithMultipleQueryParameters) {
    http::router r;
    auto handler = [](const http::context&) { return "Test"; };
    r.register_handler("GET", "/users", handler);

    http::request::context request = {};
    request.method = "GET";
    request.path = "/users?name=John&age=30&city=NYC";

    http::context ctx = {request};

    http::request_handler out_handler;
    ASSERT_TRUE(r.match(&ctx, &out_handler));
    EXPECT_EQ(3, ctx.request.query.size());
    EXPECT_EQ("John", ctx.request.query["name"]);
    EXPECT_EQ("30", ctx.request.query["age"]);
    EXPECT_EQ("NYC", ctx.request.query["city"]);
}

TEST_F(ServerTestFixture, MatchWithPathParametersAndQueryParameters) {
    http::router r;
    auto handler = [](const http::context&) { return "Test"; };
    r.register_handler("GET", "/users/:id", handler);

    http::request::context request = {};
    request.method = "GET";
    request.path = "/users/123?format=json&include=posts";

    http::context ctx = {request};

    http::request_handler out_handler;
    ASSERT_TRUE(r.match(&ctx, &out_handler));
    EXPECT_EQ(1, ctx.request.params.size());
    EXPECT_EQ("123", ctx.request.params["id"]);
    EXPECT_EQ(2, ctx.request.query.size());
    EXPECT_EQ("json", ctx.request.query["format"]);
    EXPECT_EQ("posts", ctx.request.query["include"]);
}

TEST_F(ServerTestFixture, MatchWithEmptyQueryParameters) {
    http::router r;
    auto handler = [](const http::context&) { return "Test"; };
    r.register_handler("GET", "/users", handler);

    http::request::context request = {};
    request.method = "GET";
    request.path = "/users?";

    http::context ctx = {request};

    http::request_handler out_handler;
    ASSERT_TRUE(r.match(&ctx, &out_handler));
    EXPECT_EQ(0, ctx.request.query.size());
}

TEST_F(ServerTestFixture, MatchWithQueryParameterWithoutValue) {
    http::router r;
    auto handler = [](const http::context&) { return "Test"; };
    r.register_handler("GET", "/users", handler);

    http::request::context request = {};
    request.method = "GET";
    request.path = "/users?name";

    http::context ctx = {request};

    http::request_handler out_handler;
    ASSERT_TRUE(r.match(&ctx, &out_handler));
    // Note: Empty query parameters are not stored in the map
    EXPECT_EQ(0, ctx.request.query.size());
}

TEST_F(ServerTestFixture, MatchWithSpecialCharactersInQueryParameters) {
    http::router r;
    auto handler = [](const http::context&) { return "Test"; };
    r.register_handler("GET", "/search", handler);

    http::request::context request = {};
    request.method = "GET";
    request.path = "/search?q=hello%20world&category=tech%2Bdev";

    http::context ctx = {request};

    http::request_handler out_handler;
    ASSERT_TRUE(r.match(&ctx, &out_handler));
    EXPECT_EQ(2, ctx.request.query.size());
    EXPECT_EQ("hello%20world", ctx.request.query["q"]);
    EXPECT_EQ("tech%2Bdev", ctx.request.query["category"]);
}

TEST_F(ServerTestFixture, MatchWithRepeatedQueryParameterNames) {
    http::router r;
    auto handler = [](const http::context&) { return "Test"; };
    r.register_handler("GET", "/users", handler);

    http::request::context request = {};
    request.method = "GET";
    request.path = "/users?filter=active&filter=verified";

    http::context ctx = {request};

    http::request_handler out_handler;
    ASSERT_TRUE(r.match(&ctx, &out_handler));
    EXPECT_EQ(1, ctx.request.query.size());
    EXPECT_EQ("verified", ctx.request.query["filter"]);  // Last value wins
}

TEST_F(ServerTestFixture, MatchWithNoQueryParameters) {
    http::router r;
    auto handler = [](const http::context&) { return "Test"; };
    r.register_handler("GET", "/users/:id", handler);

    http::request::context request = {};
    request.method = "GET";
    request.path = "/users/123";

    http::context ctx = {request};

    http::request_handler out_handler;
    ASSERT_TRUE(r.match(&ctx, &out_handler));
    EXPECT_EQ(1, ctx.request.params.size());
    EXPECT_EQ("123", ctx.request.params["id"]);
    EXPECT_EQ(0, ctx.request.query.size());
}
