#include <gtest/gtest.h>

#include "server.h"

TEST(MimeTypeTest, KnownExtensions) {
    http::Request req;
    EXPECT_EQ(req.get_mime_type("index.html"), http::content_type::HTML);
    EXPECT_EQ(req.get_mime_type("style.css"), http::content_type::CSS);
    EXPECT_EQ(req.get_mime_type("script.js"), http::content_type::JavaScript);
    EXPECT_EQ(req.get_mime_type("image.jpg"), http::content_type::JPEG);
    EXPECT_EQ(req.get_mime_type("photo.png"), http::content_type::PNG);
}

TEST(MimeTypeTest, UnknownExtensions) {
    http::Request req;
    EXPECT_EQ(req.get_mime_type("unknown.xyz"), "");
    EXPECT_EQ(req.get_mime_type("file"), "");
    EXPECT_EQ(req.get_mime_type(""), "");
}

TEST(MimeTypeTest, EdgeCases) {
    http::Request req;
    EXPECT_EQ(req.get_mime_type(".hidden"), "");
    EXPECT_EQ(req.get_mime_type("."), "");
    EXPECT_EQ(req.get_mime_type("file."), "");
}

TEST(ParseRequestTest, ValidGetRequest) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/index.html");
    EXPECT_EQ(req.get_mime_type("/index.html"), http::content_type::HTML);
}

TEST(ParseRequestTest, ValidPostRequest) {
    http::Request req;
    req.set_method("POST");
    req.set_path("/api/data");
    EXPECT_EQ(req.get_mime_type("/api/data"), "");
}

TEST(ParseRequestTest, ValidRequestWithQueryParams) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/page.html?param=value");
    EXPECT_EQ(req.get_mime_type("/page.html?param=value"), http::content_type::HTML);
}

TEST(ParseRequestTest, ValidRequestWithDirectoryPath) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/assets/css/style.css");
    EXPECT_EQ(req.get_mime_type("/assets/css/style.css"), http::content_type::CSS);
}

TEST(ParseRequestTest, ValidRequestWithNoExtension) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/api/users");
    EXPECT_EQ(req.get_mime_type("/api/users"), "");
}

TEST(ParseRequestTest, ValidRequestWithJsonExtension) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/data.json");
    EXPECT_EQ(req.get_mime_type("/data.json"), http::content_type::JSON);
}

TEST(ParseRequestTest, ValidRequestWithJsExtension) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/script.js");
    EXPECT_EQ(req.get_mime_type("/script.js"), http::content_type::JavaScript);
}

TEST(ParseRequestTest, ValidRequestWithMp4Extension) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/video.mp4");
    EXPECT_EQ(req.get_mime_type("/video.mp4"), http::content_type::MP4);
}

TEST(ParseRequestTest, EmptyRequest) {
    http::Request req;
    req.set_method("");
    req.set_path("");
    EXPECT_EQ(req.get_mime_type(""), "");
}

TEST(ParseRequestTest, RequestWithOnlyMethod) {
    http::Request req;
    req.set_method("GET");
    req.set_path("");
    EXPECT_EQ(req.get_mime_type(""), "");
}

TEST(ParseRequestTest, RequestWithOnlyMethodAndPath) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/index.html");
    EXPECT_EQ(req.get_mime_type("/index.html"), http::content_type::HTML);
}

TEST(ParseRequestTest, ValidRequestWithDifferentHttpVersion) {
    http::Request req;
    req.set_method("GET");
    req.set_path("/index.html");

    EXPECT_EQ(req.get_mime_type("/index.html"), http::content_type::HTML);
}
