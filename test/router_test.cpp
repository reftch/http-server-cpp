
#include "server.hpp"
#include "server_test.hpp"

TEST_F(ServerTestFixture, RegisterHandlerSucceeds) {
    // Arrange
    http::router r;
    std::string path = "/api/status";
    std::string method = "GET";

    auto handler = [](const http::request& req) {
        return "Test " + req.getPath();
    };

    r.register_handler(method, path, handler);

    // Create a request
    http::request req;
    req.setMethod(method);
    req.setPath(path);

    // Act
    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ("Test " + path, out_handler(req));
}

TEST_F(ServerTestFixture, RegisterPostHandlerSucceeds) {
    // Arrange
    http::router r;
    std::string path = "/api/v1/users";
    std::string method = "POST";

    auto handler = [](const http::request& req) {
        return "Test " + req.getPath();
    };
    r.register_handler(method, path, handler);

    // Act
    http::request req;
    req.setMethod(method);
    req.setPath(path);

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ("Test " + path, out_handler(req));
}

TEST_F(ServerTestFixture, RegisterHandlerWithPathParameter) {
    // Arrange
    http::router r;
    std::string path = "/api/v1/users/:id";
    std::string method = "GET";

    auto handler = [](const http::request& req) {
        return "Test " + req.getPath();
    };
    r.register_handler(method, path, handler);

    // Act
    http::request req;
    req.setMethod(method);
    req.setPath("/api/v1/users/123");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ("Test /api/v1/users/123", out_handler(req));
    EXPECT_EQ("123", req.getParams().at("id"));
}

TEST_F(ServerTestFixture, MatchQueryParametersWithSpaces) {
    // Arrange
    http::router r;
    auto handler = [](const http::request&) {
        return "Test";
    };
    r.register_handler("GET", "/search", handler);

    // Act
    http::request req;
    req.setMethod("GET");
    req.setPath("/search?query=hello world&filter=category 1");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(2, req.getQuery().size());
    EXPECT_EQ("hello world", req.getQuery().at("query"));
    EXPECT_EQ("category 1", req.getQuery().at("filter"));
}

TEST_F(ServerTestFixture, MatchWithSingleQueryParameter) {
    // Arrange
    http::router r;
    auto handler = [](const http::request&) {
        return "Test";
    };
    r.register_handler("GET", "/users", handler);

    // Act
    http::request req;
    req.setMethod("GET");
    req.setPath("/users?name=John");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(1, req.getQuery().size());
    EXPECT_EQ("John", req.getQuery().at("name"));
}

TEST_F(ServerTestFixture, MatchWithMultipleQueryParameters) {
    // Arrange
    http::router r;
    auto handler = [](const http::request&) {
        return "Test";
    };
    r.register_handler("GET", "/users", handler);

    // Act
    http::request req;
    req.setMethod("GET");
    req.setPath("/users?name=John&age=30&city=NYC");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(3, req.getQuery().size());
    EXPECT_EQ("John", req.getQuery().at("name"));
    EXPECT_EQ("30", req.getQuery().at("age"));
    EXPECT_EQ("NYC", req.getQuery().at("city"));
}

TEST_F(ServerTestFixture, MatchWithPathParametersAndQueryParameters) {
    // Arrange
    http::router r;
    auto handler = [](const http::request&) {
        return "Test";
    };
    r.register_handler("GET", "/users/:id", handler);

    // Act
    http::request req;
    req.setMethod("GET");
    req.setPath("/users/123?format=json&include=posts");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(1, req.getParams().size());
    EXPECT_EQ("123", req.getParams().at("id"));
    EXPECT_EQ(2, req.getQuery().size());
    EXPECT_EQ("json", req.getQuery().at("format"));
    EXPECT_EQ("posts", req.getQuery().at("include"));
}

TEST_F(ServerTestFixture, MatchWithEmptyQueryParameters) {
    // Arrange
    http::router r;
    auto handler = [](const http::request&) {
        return "Test";
    };
    r.register_handler("GET", "/users", handler);

    // Act
    http::request req;
    req.setMethod("GET");
    req.setPath("/users?");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(0, req.getQuery().size());
}

TEST_F(ServerTestFixture, MatchWithQueryParameterWithoutValue) {
    // Arrange
    http::router r;
    auto handler = [](const http::request&) {
        return "Test";
    };
    r.register_handler("GET", "/users", handler);

    // Act
    http::request req;
    req.setMethod("GET");
    req.setPath("/users?name");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(0, req.getQuery().size());  // Empty params are ignored
}

TEST_F(ServerTestFixture, MatchWithSpecialCharactersInQueryParameters) {
    // Arrange
    http::router r;
    auto handler = [](const http::request&) {
        return "Test";
    };
    r.register_handler("GET", "/search", handler);

    // Act
    http::request req;
    req.setMethod("GET");
    req.setPath("/search?q=hello%20world&category=tech%2Bdev");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(2, req.getQuery().size());
    EXPECT_EQ("hello%20world", req.getQuery().at("q"));
    EXPECT_EQ("tech%2Bdev", req.getQuery().at("category"));
}

TEST_F(ServerTestFixture, MatchWithRepeatedQueryParameterNames) {
    // Arrange
    http::router r;
    auto handler = [](const http::request&) {
        return "Test";
    };
    r.register_handler("GET", "/users", handler);

    // Act
    http::request req;
    req.setMethod("GET");
    req.setPath("/users?filter=active&filter=verified");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(1, req.getQuery().size());
    EXPECT_EQ("verified", req.getQuery().at("filter"));  // Last value wins
}

TEST_F(ServerTestFixture, MatchWithNoQueryParameters) {
    // Arrange
    http::router r;
    auto handler = [](const http::request&) {
        return "Test";
    };
    r.register_handler("GET", "/users/:id", handler);

    // Act
    http::request req;
    req.setMethod("GET");
    req.setPath("/users/123");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(1, req.getParams().size());
    EXPECT_EQ("123", req.getParams().at("id"));
    EXPECT_EQ(0, req.getQuery().size());
}
