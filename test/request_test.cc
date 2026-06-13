#include <gtest/gtest.h>

#include "server.h"

TEST(MimeTypeTest, KnownExtensions) {
    http::Request req;
    EXPECT_EQ(req.getMimeType("index.html"), "text/html");
    EXPECT_EQ(req.getMimeType("style.css"), "text/css");
    EXPECT_EQ(req.getMimeType("script.js"), "text/javascript");
    EXPECT_EQ(req.getMimeType("image.jpg"), "image/jpeg");
    EXPECT_EQ(req.getMimeType("photo.png"), "image/png");
}

TEST(MimeTypeTest, UnknownExtensions) {
    http::Request req;
    EXPECT_EQ(req.getMimeType("unknown.xyz"), "");
    EXPECT_EQ(req.getMimeType("file"), "");
    EXPECT_EQ(req.getMimeType(""), "");
}

TEST(MimeTypeTest, EdgeCases) {
    http::Request req;
    EXPECT_EQ(req.getMimeType(".hidden"), "");
    EXPECT_EQ(req.getMimeType("."), "");
    EXPECT_EQ(req.getMimeType("file."), "");
}

TEST(ParseRequestTest, ValidGetRequest) {
    http::Request req;
    req.setMethod("GET");
    req.setPath("/index.html");
    EXPECT_EQ(req.getMimeType("/index.html"), "text/html");
}

TEST(ParseRequestTest, ValidPostRequest) {
    http::Request req;
    req.setMethod("POST");
    req.setPath("/api/data");
    EXPECT_EQ(req.getMimeType("/api/data"), "");
}

TEST(ParseRequestTest, ValidRequestWithQueryParams) {
    http::Request req;
    req.setMethod("GET");
    req.setPath("/page.html?param=value");
    EXPECT_EQ(req.getMimeType("/page.html?param=value"), "text/html");
}

TEST(ParseRequestTest, ValidRequestWithDirectoryPath) {
    http::Request req;
    req.setMethod("GET");
    req.setPath("/assets/css/style.css");
    EXPECT_EQ(req.getMimeType("/assets/css/style.css"), "text/css");
}

TEST(ParseRequestTest, ValidRequestWithNoExtension) {
    http::Request req;
    req.setMethod("GET");
    req.setPath("/api/users");
    EXPECT_EQ(req.getMimeType("/api/users"), "");
}

TEST(ParseRequestTest, ValidRequestWithJsonExtension) {
    http::Request req;
    req.setMethod("GET");
    req.setPath("/data.json");
    EXPECT_EQ(req.getMimeType("/data.json"), "application/json");
}

TEST(ParseRequestTest, ValidRequestWithJsExtension) {
    http::Request req;
    req.setMethod("GET");
    req.setPath("/script.js");
    EXPECT_EQ(req.getMimeType("/script.js"), "text/javascript");
}

TEST(ParseRequestTest, ValidRequestWithMp4Extension) {
    http::Request req;
    req.setMethod("GET");
    req.setPath("/video.mp4");
    EXPECT_EQ(req.getMimeType("/video.mp4"), "video/mp4");
}

TEST(ParseRequestTest, EmptyRequest) {
    http::Request req;
    req.setMethod("");
    req.setPath("");
    EXPECT_EQ(req.getMimeType(""), "");
}

TEST(ParseRequestTest, RequestWithOnlyMethod) {
    http::Request req;
    req.setMethod("GET");
    req.setPath("");
    EXPECT_EQ(req.getMimeType(""), "");
}

TEST(ParseRequestTest, RequestWithOnlyMethodAndPath) {
    http::Request req;
    req.setMethod("GET");
    req.setPath("/index.html");
    EXPECT_EQ(req.getMimeType("/index.html"), "text/html");
}

TEST(ParseRequestTest, ValidRequestWithDifferentHttpVersion) {
    http::Request req;
    req.setMethod("GET");
    req.setPath("/index.html");

    EXPECT_EQ(req.getMimeType("/index.html"), "text/html");
}

// Additional tests for case insensitivity
TEST(MimeTypeTest, CaseInsensitiveExtensions) {
    http::Request req;
    EXPECT_EQ(req.getMimeType("test.HTML"), "text/html");
    EXPECT_EQ(req.getMimeType("test.JPEG"), "image/jpeg");
    EXPECT_EQ(req.getMimeType("test.Png"), "image/png");
}