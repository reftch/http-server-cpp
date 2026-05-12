#include <gtest/gtest.h>

#include "server.hpp"

TEST(MimeTypeTest, KnownExtensions) {
    EXPECT_EQ(http::request::get_mime_type("index.html"), http::response::content_type::HTML);
    EXPECT_EQ(http::request::get_mime_type("style.css"), http::response::content_type::CSS);
    EXPECT_EQ(http::request::get_mime_type("script.js"), http::response::content_type::JavaScript);
    EXPECT_EQ(http::request::get_mime_type("image.jpg"), http::response::content_type::JPEG);
    EXPECT_EQ(http::request::get_mime_type("photo.png"), http::response::content_type::PNG);
}

TEST(MimeTypeTest, UnknownExtensions) {
    EXPECT_EQ(http::request::get_mime_type("unknown.xyz"), "");
    EXPECT_EQ(http::request::get_mime_type("file"), "");
    EXPECT_EQ(http::request::get_mime_type(""), "");
}

TEST(MimeTypeTest, EdgeCases) {
    EXPECT_EQ(http::request::get_mime_type(".hidden"), "");
    EXPECT_EQ(http::request::get_mime_type("."), "");
    EXPECT_EQ(http::request::get_mime_type("file."), "");
}

TEST(ParseRequestTest, ValidGetRequest) {
    std::string raw_request = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
    auto req = http::request::parse(raw_request);

    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/index.html");
    EXPECT_EQ(req.version, "HTTP/1.1");
    EXPECT_EQ(req.mime_type, http::response::content_type::HTML);
}

TEST(ParseRequestTest, ValidPostRequest) {
    std::string raw_request = "POST /api/data HTTP/1.1\r\nHost: localhost\r\n\r\n";
    auto req = http::request::parse(raw_request);

    EXPECT_EQ(req.method, "POST");
    EXPECT_EQ(req.path, "/api/data");
    EXPECT_EQ(req.version, "HTTP/1.1");
    EXPECT_EQ(req.mime_type, "");
}

TEST(ParseRequestTest, ValidRequestWithQueryParams) {
    std::string raw_request = "GET /page.html?param=value HTTP/1.1\r\nHost: localhost\r\n\r\n";
    auto req = http::request::parse(raw_request);

    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/page.html?param=value");
    EXPECT_EQ(req.version, "HTTP/1.1");
    EXPECT_EQ(req.mime_type, http::response::content_type::HTML);
}

TEST(ParseRequestTest, ValidRequestWithDirectoryPath) {
    std::string raw_request = "GET /assets/css/style.css HTTP/1.1\r\nHost: localhost\r\n\r\n";
    auto req = http::request::parse(raw_request);

    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/assets/css/style.css");
    EXPECT_EQ(req.version, "HTTP/1.1");
    EXPECT_EQ(req.mime_type, http::response::content_type::CSS);
}

TEST(ParseRequestTest, ValidRequestWithNoExtension) {
    std::string raw_request = "GET /api/users HTTP/1.1\r\nHost: localhost\r\n\r\n";
    auto req = http::request::parse(raw_request);

    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/api/users");
    EXPECT_EQ(req.version, "HTTP/1.1");
    EXPECT_EQ(req.mime_type, "");
}

TEST(ParseRequestTest, ValidRequestWithJsonExtension) {
    std::string raw_request = "GET /data.json HTTP/1.1\r\nHost: localhost\r\n\r\n";
    auto req = http::request::parse(raw_request);

    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/data.json");
    EXPECT_EQ(req.version, "HTTP/1.1");
    EXPECT_EQ(req.mime_type, http::response::content_type::JSON);
}

TEST(ParseRequestTest, ValidRequestWithJsExtension) {
    std::string raw_request = "GET /script.js HTTP/1.1\r\nHost: localhost\r\n\r\n";
    auto req = http::request::parse(raw_request);

    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/script.js");
    EXPECT_EQ(req.version, "HTTP/1.1");
    EXPECT_EQ(req.mime_type, http::response::content_type::JavaScript);
}

TEST(ParseRequestTest, ValidRequestWithMp4Extension) {
    std::string raw_request = "GET /video.mp4 HTTP/1.1\r\nHost: localhost\r\n\r\n";
    auto req = http::request::parse(raw_request);

    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/video.mp4");
    EXPECT_EQ(req.version, "HTTP/1.1");
    EXPECT_EQ(req.mime_type, http::response::content_type::MP4);
}

TEST(ParseRequestTest, EmptyRequest) {
    std::string raw_request = "";
    auto req = http::request::parse(raw_request);

    EXPECT_EQ(req.method, "");
    EXPECT_EQ(req.path, "");
    EXPECT_EQ(req.version, "");
    EXPECT_EQ(req.mime_type, "");
}

TEST(ParseRequestTest, RequestWithOnlyMethod) {
    std::string raw_request = "GET\r\nHost: localhost\r\n\r\n";
    auto req = http::request::parse(raw_request);

    EXPECT_EQ(req.method, "");
    EXPECT_EQ(req.path, "");
    EXPECT_EQ(req.version, "");
    EXPECT_EQ(req.mime_type, "");
}

TEST(ParseRequestTest, RequestWithOnlyMethodAndPath) {
    std::string raw_request = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
    auto req = http::request::parse(raw_request);

    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/index.html");
    EXPECT_EQ(req.version, "HTTP/1.1");
    EXPECT_EQ(req.mime_type, http::response::content_type::HTML);
}

TEST(ParseRequestTest, ValidRequestWithDifferentHttpVersion) {
    std::string raw_request = "GET /index.html HTTP/2.0\r\nHost: localhost\r\n\r\n";
    auto req = http::request::parse(raw_request);

    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/index.html");
    EXPECT_EQ(req.version, "HTTP/2.0");
    EXPECT_EQ(req.mime_type, http::response::content_type::HTML);
}