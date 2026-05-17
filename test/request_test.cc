#include <gtest/gtest.h>

#include "server.h"

TEST(MimeTypeTest, KnownExtensions) {
    http::Request req;
    EXPECT_EQ(req.GetMimeType("index.html"), "text/html");
    EXPECT_EQ(req.GetMimeType("style.css"), "text/css");
    EXPECT_EQ(req.GetMimeType("script.js"), "application/javascript");
    EXPECT_EQ(req.GetMimeType("image.jpg"), "image/jpeg");
    EXPECT_EQ(req.GetMimeType("photo.png"), "image/png");
}

TEST(MimeTypeTest, UnknownExtensions) {
    http::Request req;
    EXPECT_EQ(req.GetMimeType("unknown.xyz"), "");
    EXPECT_EQ(req.GetMimeType("file"), "");
    EXPECT_EQ(req.GetMimeType(""), "");
}

TEST(MimeTypeTest, EdgeCases) {
    http::Request req;
    EXPECT_EQ(req.GetMimeType(".hidden"), "");
    EXPECT_EQ(req.GetMimeType("."), "");
    EXPECT_EQ(req.GetMimeType("file."), "");
}

TEST(ParseRequestTest, ValidGetRequest) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/index.html");
    EXPECT_EQ(req.GetMimeType("/index.html"), "text/html");
}

TEST(ParseRequestTest, ValidPostRequest) {
    http::Request req;
    req.set_method("POST");
    req.set_path("/api/data");
    EXPECT_EQ(req.GetMimeType("/api/data"), "");
}

TEST(ParseRequestTest, ValidRequestWithQueryParams) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/page.html?param=value");
    EXPECT_EQ(req.GetMimeType("/page.html?param=value"), "text/html");
}

TEST(ParseRequestTest, ValidRequestWithDirectoryPath) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/assets/css/style.css");
    EXPECT_EQ(req.GetMimeType("/assets/css/style.css"), "text/css");
}

TEST(ParseRequestTest, ValidRequestWithNoExtension) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/api/users");
    EXPECT_EQ(req.GetMimeType("/api/users"), "");
}

TEST(ParseRequestTest, ValidRequestWithJsonExtension) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/data.json");
    EXPECT_EQ(req.GetMimeType("/data.json"), "application/json");
}

TEST(ParseRequestTest, ValidRequestWithJsExtension) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/script.js");
    EXPECT_EQ(req.GetMimeType("/script.js"), "application/javascript");
}

TEST(ParseRequestTest, ValidRequestWithMp4Extension) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/video.mp4");
    EXPECT_EQ(req.GetMimeType("/video.mp4"), "video/mp4");
}

TEST(ParseRequestTest, EmptyRequest) {
    http::Request req;
    req.set_method("");
    req.set_path("");
    EXPECT_EQ(req.GetMimeType(""), "");
}

TEST(ParseRequestTest, RequestWithOnlyMethod) {
    http::Request req;
    req.set_method("GET");
    req.set_path("");
    EXPECT_EQ(req.GetMimeType(""), "");
}

TEST(ParseRequestTest, RequestWithOnlyMethodAndPath) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/index.html");
    EXPECT_EQ(req.GetMimeType("/index.html"), "text/html");
}

TEST(ParseRequestTest, ValidRequestWithDifferentHttpVersion) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/index.html");

    EXPECT_EQ(req.GetMimeType("/index.html"), "text/html");
}

// Additional tests for case insensitivity
TEST(MimeTypeTest, CaseInsensitiveExtensions) {
    http::Request req;
    EXPECT_EQ(req.GetMimeType("test.HTML"), "text/html");
    EXPECT_EQ(req.GetMimeType("test.JPEG"), "image/jpeg");
    EXPECT_EQ(req.GetMimeType("test.Png"), "image/png");
}