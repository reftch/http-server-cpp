#include <gtest/gtest.h>

#include "server.hpp"

TEST(MimeTypeTest, KnownExtensions) {
    http::request req;
    EXPECT_EQ(req.get_mime_type("index.html"), http::response::content_type::HTML);
    EXPECT_EQ(req.get_mime_type("style.css"), http::response::content_type::CSS);
    EXPECT_EQ(req.get_mime_type("script.js"), http::response::content_type::JavaScript);
    EXPECT_EQ(req.get_mime_type("image.jpg"), http::response::content_type::JPEG);
    EXPECT_EQ(req.get_mime_type("photo.png"), http::response::content_type::PNG);
}

TEST(MimeTypeTest, UnknownExtensions) {
    http::request req;
    EXPECT_EQ(req.get_mime_type("unknown.xyz"), "");
    EXPECT_EQ(req.get_mime_type("file"), "");
    EXPECT_EQ(req.get_mime_type(""), "");
}

TEST(MimeTypeTest, EdgeCases) {
    http::request req;
    EXPECT_EQ(req.get_mime_type(".hidden"), "");
    EXPECT_EQ(req.get_mime_type("."), "");
    EXPECT_EQ(req.get_mime_type("file."), "");
}

TEST(ParseRequestTest, ValidGetRequest) {
    http::request req;
    req.setMethod("GET");
    req.setPath("/index.html");
    EXPECT_EQ(req.get_mime_type("/index.html"), http::response::content_type::HTML);
}

TEST(ParseRequestTest, ValidPostRequest) {
    http::request req;
    req.setMethod("POST");
    req.setPath("/api/data");
    EXPECT_EQ(req.get_mime_type("/api/data"), "");
}

TEST(ParseRequestTest, ValidRequestWithQueryParams) {
    http::request req;
    req.setMethod("GET");
    req.setPath("/page.html?param=value");
    EXPECT_EQ(req.get_mime_type("/page.html?param=value"), http::response::content_type::HTML);
}

TEST(ParseRequestTest, ValidRequestWithDirectoryPath) {
    http::request req;
    req.setMethod("GET");
    req.setPath("/assets/css/style.css");
    EXPECT_EQ(req.get_mime_type("/assets/css/style.css"), http::response::content_type::CSS);
}

TEST(ParseRequestTest, ValidRequestWithNoExtension) {
    http::request req;
    req.setMethod("GET");
    req.setPath("/api/users");
    EXPECT_EQ(req.get_mime_type("/api/users"), "");
}

TEST(ParseRequestTest, ValidRequestWithJsonExtension) {
    http::request req;
    req.setMethod("GET");
    req.setPath("/data.json");
    EXPECT_EQ(req.get_mime_type("/data.json"), http::response::content_type::JSON);
}

TEST(ParseRequestTest, ValidRequestWithJsExtension) {
    http::request req;
    req.setMethod("GET");
    req.setPath("/script.js");
    EXPECT_EQ(req.get_mime_type("/script.js"), http::response::content_type::JavaScript);
}

TEST(ParseRequestTest, ValidRequestWithMp4Extension) {
    http::request req;
    req.setMethod("GET");
    req.setPath("/video.mp4");
    EXPECT_EQ(req.get_mime_type("/video.mp4"), http::response::content_type::MP4);
}

TEST(ParseRequestTest, EmptyRequest) {
    http::request req;
    req.setMethod("");
    req.setPath("");
    EXPECT_EQ(req.get_mime_type(""), "");
}

TEST(ParseRequestTest, RequestWithOnlyMethod) {
    http::request req;
    req.setMethod("GET");
    req.setPath("");
    EXPECT_EQ(req.get_mime_type(""), "");
}

TEST(ParseRequestTest, RequestWithOnlyMethodAndPath) {
    http::request req;
    req.setMethod("GET");
    req.setPath("/index.html");
    EXPECT_EQ(req.get_mime_type("/index.html"), http::response::content_type::HTML);
}

TEST(ParseRequestTest, ValidRequestWithDifferentHttpVersion) {
    http::request req;
    req.setMethod("GET");
    req.setPath("/index.html");

    EXPECT_EQ(req.get_mime_type("/index.html"), http::response::content_type::HTML);
}
