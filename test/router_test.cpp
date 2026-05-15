
#include "server.hpp"
#include "server_test.hpp"

TEST_F(ServerTestFixture, RegisterHandlerSucceeds) {
    // Arrange
    http::router r;
    std::string path = "/api/status";
    std::string method = "GET";

    auto handler = [](const http::Request& req) {
        return "Test " + req.path();
    };

    r.register_handler(method, path, handler);

    // Create a request
    http::Request req;
    req.set_method(method);
    req.set_path(path);

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

    auto handler = [](const http::Request& req) {
        return "Test " + req.path();
    };
    r.register_handler(method, path, handler);

    // Act
    http::Request req;
    req.set_method(method);
    req.set_path(path);

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

    auto handler = [](const http::Request& req) {
        return "Test " + req.path();
    };
    r.register_handler(method, path, handler);

    // Act
    http::Request req;
    req.set_method(method);
    req.set_path("/api/v1/users/123");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ("Test /api/v1/users/123", out_handler(req));
    EXPECT_EQ("123", req.params().at("id"));
}

TEST_F(ServerTestFixture, MatchQueryParametersWithSpaces) {
    // Arrange
    http::router r;
    auto handler = [](const http::Request&) {
        return "Test";
    };
    r.register_handler("GET", "/search", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/search?query=hello world&filter=category 1");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(2, req.query().size());
    EXPECT_EQ("hello world", req.query().at("query"));
    EXPECT_EQ("category 1", req.query().at("filter"));
}

TEST_F(ServerTestFixture, MatchWithSingleQueryParameter) {
    // Arrange
    http::router r;
    auto handler = [](const http::Request&) {
        return "Test";
    };
    r.register_handler("GET", "/users", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/users?name=John");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(1, req.query().size());
    EXPECT_EQ("John", req.query().at("name"));
}

TEST_F(ServerTestFixture, MatchWithMultipleQueryParameters) {
    // Arrange
    http::router r;
    auto handler = [](const http::Request&) {
        return "Test";
    };
    r.register_handler("GET", "/users", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/users?name=John&age=30&city=NYC");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(3, req.query().size());
    EXPECT_EQ("John", req.query().at("name"));
    EXPECT_EQ("30", req.query().at("age"));
    EXPECT_EQ("NYC", req.query().at("city"));
}

TEST_F(ServerTestFixture, MatchWithPathParametersAndQueryParameters) {
    // Arrange
    http::router r;
    auto handler = [](const http::Request&) {
        return "Test";
    };
    r.register_handler("GET", "/users/:id", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/users/123?format=json&include=posts");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(1, req.params().size());
    EXPECT_EQ("123", req.params().at("id"));
    EXPECT_EQ(2, req.query().size());
    EXPECT_EQ("json", req.query().at("format"));
    EXPECT_EQ("posts", req.query().at("include"));
}

TEST_F(ServerTestFixture, MatchWithEmptyQueryParameters) {
    // Arrange
    http::router r;
    auto handler = [](const http::Request&) {
        return "Test";
    };
    r.register_handler("GET", "/users", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/users?");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(0, req.query().size());
}

TEST_F(ServerTestFixture, MatchWithQueryParameterWithoutValue) {
    // Arrange
    http::router r;
    auto handler = [](const http::Request&) {
        return "Test";
    };
    r.register_handler("GET", "/users", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/users?name");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(0, req.query().size());  // Empty params are ignored
}

TEST_F(ServerTestFixture, MatchWithSpecialCharactersInQueryParameters) {
    // Arrange
    http::router r;
    auto handler = [](const http::Request&) {
        return "Test";
    };
    r.register_handler("GET", "/search", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/search?q=hello%20world&category=tech%2Bdev");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(2, req.query().size());
    EXPECT_EQ("hello%20world", req.query().at("q"));
    EXPECT_EQ("tech%2Bdev", req.query().at("category"));
}

TEST_F(ServerTestFixture, MatchWithRepeatedQueryParameterNames) {
    // Arrange
    http::router r;
    auto handler = [](const http::Request&) {
        return "Test";
    };
    r.register_handler("GET", "/users", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/users?filter=active&filter=verified");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(1, req.query().size());
    EXPECT_EQ("verified", req.query().at("filter"));  // Last value wins
}

TEST_F(ServerTestFixture, MatchWithNoQueryParameters) {
    // Arrange
    http::router r;
    auto handler = [](const http::Request&) {
        return "Test";
    };
    r.register_handler("GET", "/users/:id", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/users/123");

    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    EXPECT_EQ(1, req.params().size());
    EXPECT_EQ("123", req.params().at("id"));
    EXPECT_EQ(0, req.query().size());
}
