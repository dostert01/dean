#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "http_request.hpp"

TEST_CASE("setUrl - path only, no query string") {
    HTTPRequest req;
    req.setUrl("/api/users");

    CHECK(req.path()   == "/api/users");
    CHECK(req.rawUrl() == "/api/users");
    CHECK(req.queryParams().empty());
}

TEST_CASE("setUrl - path with single query parameter") {
    HTTPRequest req;
    req.setUrl("/search?q=hello");

    CHECK(req.path() == "/search");
    REQUIRE(req.queryParam("q").has_value());
    CHECK(req.queryParam("q").value() == "hello");
}

TEST_CASE("setUrl - path with multiple query parameters") {
    HTTPRequest req;
    req.setUrl("/items?page=2&limit=50&sort=asc");

    CHECK(req.path() == "/items");
    CHECK(req.queryParam("page").value()  == "2");
    CHECK(req.queryParam("limit").value() == "50");
    CHECK(req.queryParam("sort").value()  == "asc");
}

TEST_CASE("setUrl - value-less flag parameter") {
    HTTPRequest req;
    req.setUrl("/export?verbose");

    CHECK(req.path() == "/export");
    REQUIRE(req.queryParam("verbose").has_value());
    CHECK(req.queryParam("verbose").value() == "");
}

TEST_CASE("setUrl - missing key returns nullopt") {
    HTTPRequest req;
    req.setUrl("/ping?foo=bar");

    CHECK_FALSE(req.queryParam("baz").has_value());
}

TEST_CASE("setUrl - empty query string after '?'") {
    HTTPRequest req;
    req.setUrl("/ping?");

    CHECK(req.path() == "/ping");
    CHECK(req.queryParams().empty());
}

TEST_CASE("setUrl - root path") {
    HTTPRequest req;
    req.setUrl("/");

    CHECK(req.path() == "/");
    CHECK(req.queryParams().empty());
}

TEST_CASE("setUrl - parameter with '=' in value") {
    HTTPRequest req;
    req.setUrl("/decode?token=abc=def");

    CHECK(req.path() == "/decode");
    // Only splits on the first '=', remainder is part of value
    CHECK(req.queryParam("token").value() == "abc=def");
}

TEST_CASE("setUrl - rawUrl preserves original string") {
    const std::string url = "/api/v1/resource?id=42&active=true";
    HTTPRequest req;
    req.setUrl(url);

    CHECK(req.rawUrl() == url);
}
