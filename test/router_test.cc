
#include "server.h"
#include "server_test.h"

TEST_F(ServerTestFixture, RegisterRootHandler) {
    // Arrange
    http::Router r;
    std::string path = "/";
    std::string method = "GET";

    auto handler = [](const http::Request& req, http::Response& res) {
        res.setContent<http::ContentType::PLAIN_TEXT>("Test " + req.path());
    };

    r.registerHandler(method, path, handler);

    // Create a request
    http::Request req;
    req.setMethod(method);
    req.setPath(path);

    http::Response res;

    // Act
    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    out_handler(req, res);
    EXPECT_EQ("Test " + path, res.content());
}

TEST_F(ServerTestFixture, RegisterHandlerSucceeds) {
    // Arrange
    http::Router r;
    std::string path = "/api/status";
    std::string method = "GET";

    auto handler = [](const http::Request& req, http::Response& res) {
        res.setContent<http::ContentType::PLAIN_TEXT>("Test " + req.path());
    };

    r.registerHandler(method, path, handler);

    // Create a request
    http::Request req;
    req.setMethod(method);
    req.setPath(path);

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
        res.setContent<http::ContentType::PLAIN_TEXT>("Test " + req.path());
    };

    r.registerHandler(method, path, handler);

    // Create a request
    http::Request req;
    req.setMethod(method);
    req.setPath(path);

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
        res.setContent<http::ContentType::PLAIN_TEXT>("Test " + req.path());
    };

    r.registerHandler(method, path, handler);

    // Create a request
    http::Request req;
    req.setMethod(method);
    req.setPath("/api/v1/users/123");

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

TEST_F(ServerTestFixture, RegisterHandlerWithMultiplePathParameter) {
    // Arrange
    http::Router r;
    std::string path = "/api/v1/users/:par1/age/:par2/step/:par3";
    std::string method = "GET";

    auto handler = [](const http::Request& req, http::Response& res) {
        res.setContent<http::ContentType::PLAIN_TEXT>("Test " + req.path());
    };

    r.registerHandler(method, path, handler);

    // Create a request
    http::Request req;
    req.setMethod(method);
    req.setPath("/api/v1/users/123/age/33/step/100");

    http::Response res;

    // Act
    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    out_handler(req, res);
    EXPECT_EQ("Test /api/v1/users/123/age/33/step/100", res.content());
    EXPECT_EQ("123", req.params().at("par1"));
    EXPECT_EQ("33", req.params().at("par2"));
    EXPECT_EQ("100", req.params().at("par3"));
}

TEST_F(ServerTestFixture, RegisterHandlerWithMultipleSequencePathParameter) {
    // Arrange
    http::Router r;
    std::string path = "/api/v1/users/:par1/:par2/:par3";
    std::string method = "GET";

    auto handler = [](const http::Request& req, http::Response& res) {
        res.setContent<http::ContentType::PLAIN_TEXT>("Test " + req.path());
    };

    r.registerHandler(method, path, handler);

    // Create a request
    http::Request req;
    req.setMethod(method);
    req.setPath("/api/v1/users/123/10/100");

    http::Response res;

    // Act
    http::request_handler out_handler;
    bool matched = r.match(&req, &out_handler);

    // Assert
    ASSERT_TRUE(matched);
    out_handler(req, res);
    EXPECT_EQ("Test /api/v1/users/123/10/100", res.content());
    EXPECT_EQ("123", req.params().at("par1"));
    EXPECT_EQ("10", req.params().at("par2"));
    EXPECT_EQ("100", req.params().at("par3"));
}

TEST_F(ServerTestFixture, MatchWithSingleQueryParameter) {
    // Arrange
    http::Router r;
    auto handler = [](const http::Request&, http::Response& res) {
        res.setContent<http::ContentType::PLAIN_TEXT>("Test");
    };

    r.registerHandler("GET", "/users", handler);

    // Act
    http::Request req("GET /users?name=John HTTP/1.1 \r\n");

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
        res.setContent<http::ContentType::PLAIN_TEXT>("Test");
    };
    r.registerHandler("GET", "/users", handler);

    // Act
    http::Request req("GET /users?name=John&age=30&city=NYC HTTP/1.1 \r\n");

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
        res.setContent<http::ContentType::PLAIN_TEXT>("Test");
    };
    r.registerHandler("GET", "/users/:id", handler);

    // Act
    http::Request req("GET /users/123?format=json&include=posts HTTP/1.1 \r\n");

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
        res.setContent<http::ContentType::PLAIN_TEXT>("Test");
    };
    r.registerHandler("GET", "/users", handler);

    // Act
    http::Request req("GET /users? HTTP/1.1 \r\n");

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
        res.setContent<http::ContentType::PLAIN_TEXT>("Test");
    };
    r.registerHandler("GET", "/users", handler);

    // Act
    http::Request req("GET /users?name HTTP/1.1 \r\n");

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
        res.setContent<http::ContentType::PLAIN_TEXT>("Test");
    };
    r.registerHandler("GET", "/search", handler);

    // Act
    http::Request req("GET /search?q=hello%20world&category=tech%2Bdev HTTP/1.1 \r\n");

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
        res.setContent<http::ContentType::PLAIN_TEXT>("Test");
    };
    r.registerHandler("GET", "/users", handler);

    // Act
    http::Request req("GET /users?filter=active&filter=verified HTTP/1.1 \r\n");

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
        res.setContent<http::ContentType::PLAIN_TEXT>("Test");
    };
    r.registerHandler("GET", "/users/:id", handler);

    // Act
    http::Request req;
    req.setMethod("GET");
    req.setPath("/users/123");

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
