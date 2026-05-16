
#include "server.h"
#include "server_test.h"

TEST_F(ServerTestFixture, RegisterHandlerSucceeds) {
    // Arrange
    http::Router r;
    std::string path = "/api/status";
    std::string method = "GET";

    auto handler = [](const http::Request& req, http::Response& res) {
        res.set_content("Test " + req.path(), http::content_type::PLAIN_TEXT);
    };

    r.register_handler(method, path, handler);

    // Create a request
    http::Request req;
    req.set_method(method);
    req.set_path(path);

    http::Response res;

    // Act
    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    out_handler(req, res);
    EXPECT_EQ("Test " + path, res.content());
}

TEST_F(ServerTestFixture, RegisterPostHandlerSucceeds) {
    // Arrange
    http::Router r;
    std::string path = "/api/v1/users";
    std::string method = "POST";

    auto handler = [](const http::Request& req, http::Response& res) {
        res.set_content("Test " + req.path(), http::content_type::PLAIN_TEXT);
    };

    r.register_handler(method, path, handler);

    // Create a request
    http::Request req;
    req.set_method(method);
    req.set_path(path);

    http::Response res;

    // Act
    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    out_handler(req, res);
    EXPECT_EQ("Test " + path, res.content());
}

TEST_F(ServerTestFixture, RegisterHandlerWithPathParameter) {
    // Arrange
    http::Router r;
    std::string path = "/api/v1/users/:id";
    std::string method = "GET";

    auto handler = [](const http::Request& req, http::Response& res) {
        res.set_content("Test " + req.path(), http::content_type::PLAIN_TEXT);
    };

    r.register_handler(method, path, handler);

    // Create a request
    http::Request req;
    req.set_method(method);
    req.set_path("/api/v1/users/123");

    http::Response res;

    // Act
    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    out_handler(req, res);
    EXPECT_EQ("Test /api/v1/users/123", res.content());
    EXPECT_EQ("123", req.params().at("id"));
}

TEST_F(ServerTestFixture, MatchQueryParametersWithSpaces) {
    // Arrange
    http::Router r;
    auto handler = [](const http::Request&, http::Response& res) {
        res.set_content("Test", http::content_type::PLAIN_TEXT);
    };
    r.register_handler("GET", "/search", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/search?query=hello world&filter=category 1");

    http::Response res;
    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    out_handler(req, res);
    EXPECT_EQ(2, req.query().size());
    EXPECT_EQ("hello world", req.query().at("query"));
    EXPECT_EQ("category 1", req.query().at("filter"));
}

TEST_F(ServerTestFixture, MatchWithSingleQueryParameter) {
    // Arrange
    http::Router r;
    auto handler = [](const http::Request&, http::Response& res) {
        res.set_content("Test", http::content_type::PLAIN_TEXT);
    };
    r.register_handler("GET", "/users", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/users?name=John");

    http::Response res;
    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    out_handler(req, res);
    EXPECT_EQ(1, req.query().size());
    EXPECT_EQ("John", req.query().at("name"));
}

TEST_F(ServerTestFixture, MatchWithMultipleQueryParameters) {
    // Arrange
    http::Router r;
    auto handler = [](const http::Request&, http::Response& res) {
        res.set_content("Test", http::content_type::PLAIN_TEXT);
    };
    r.register_handler("GET", "/users", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/users?name=John&age=30&city=NYC");

    http::Response res;
    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    out_handler(req, res);
    EXPECT_EQ(3, req.query().size());
    EXPECT_EQ("John", req.query().at("name"));
    EXPECT_EQ("30", req.query().at("age"));
    EXPECT_EQ("NYC", req.query().at("city"));
}

TEST_F(ServerTestFixture, MatchWithPathParametersAndQueryParameters) {
    // Arrange
    http::Router r;
    auto handler = [](const http::Request&, http::Response& res) {
        res.set_content("Test", http::content_type::PLAIN_TEXT);
    };
    r.register_handler("GET", "/users/:id", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/users/123?format=json&include=posts");

    http::Response res;
    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    out_handler(req, res);
    EXPECT_EQ(1, req.params().size());
    EXPECT_EQ("123", req.params().at("id"));
    EXPECT_EQ(2, req.query().size());
    EXPECT_EQ("json", req.query().at("format"));
    EXPECT_EQ("posts", req.query().at("include"));
}

TEST_F(ServerTestFixture, MatchWithEmptyQueryParameters) {
    // Arrange
    http::Router r;
    auto handler = [](const http::Request&, http::Response& res) {
        res.set_content("Test", http::content_type::PLAIN_TEXT);
    };
    r.register_handler("GET", "/users", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/users?");

    http::Response res;
    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    out_handler(req, res);
    EXPECT_EQ(0, req.query().size());
}

TEST_F(ServerTestFixture, MatchWithQueryParameterWithoutValue) {
    // Arrange
    http::Router r;
    auto handler = [](const http::Request&, http::Response& res) {
        res.set_content("Test", http::content_type::PLAIN_TEXT);
    };
    r.register_handler("GET", "/users", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/users?name");

    http::Response res;
    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    out_handler(req, res);
    EXPECT_EQ(1, req.query().size());  // Empty params are ignored
}

TEST_F(ServerTestFixture, MatchWithSpecialCharactersInQueryParameters) {
    // Arrange
    http::Router r;
    auto handler = [](const http::Request&, http::Response& res) {
        res.set_content("Test", http::content_type::PLAIN_TEXT);
    };
    r.register_handler("GET", "/search", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/search?q=hello%20world&category=tech%2Bdev");

    http::Response res;
    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    out_handler(req, res);
    EXPECT_EQ(2, req.query().size());
    EXPECT_EQ("hello world", req.query().at("q"));
    EXPECT_EQ("tech+dev", req.query().at("category"));
}

TEST_F(ServerTestFixture, MatchWithRepeatedQueryParameterNames) {
    // Arrange
    http::Router r;
    auto handler = [](const http::Request&, http::Response& res) {
        res.set_content("Test", http::content_type::PLAIN_TEXT);
    };
    r.register_handler("GET", "/users", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/users?filter=active&filter=verified");

    http::Response res;
    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    out_handler(req, res);
    EXPECT_EQ(1, req.query().size());
    EXPECT_EQ("verified", req.query().at("filter"));  // Last value wins
}

TEST_F(ServerTestFixture, MatchWithNoQueryParameters) {
    // Arrange
    http::Router r;
    auto handler = [](const http::Request&, http::Response& res) {
        res.set_content("Test", http::content_type::PLAIN_TEXT);
    };
    r.register_handler("GET", "/users/:id", handler);

    // Act
    http::Request req;
    req.set_method("GET");
    req.set_path("/users/123");

    http::Response res;
    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    out_handler(req, res);
    EXPECT_EQ(1, req.params().size());
    EXPECT_EQ("123", req.params().at("id"));
    EXPECT_EQ(0, req.query().size());
}
