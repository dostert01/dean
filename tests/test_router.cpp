#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "router.hpp"

// ---- test doubles ---------------------------------------------------------

// Returns a fixed response regardless of the request
class FixedHandler : public IRequestHandler {
public:
    explicit FixedHandler(HTTPResponse response) : response_(std::move(response)) {}
    HTTPResponse handle(const HTTPRequest&) override { return response_; }
private:
    HTTPResponse response_;
};

// Records the last request it received so tests can inspect it
class CapturingHandler : public IRequestHandler {
public:
    HTTPResponse handle(const HTTPRequest& req) override {
        last_request = req;
        call_count++;
        return HTTPResponse::ok();
    }
    std::optional<HTTPRequest> last_request;
    int call_count { 0 };
};

// Always denies authorization
class ForbiddenHandler : public IRequestHandler {
public:
    bool authorize(const HTTPRequest&) override { return false; }
    HTTPResponse handle(const HTTPRequest&) override { return HTTPResponse::ok(); }
};

// ---- helpers --------------------------------------------------------------

static HTTPRequest makeRequest(HTTPMethod method, std::string url) {
    HTTPRequest req;
    req.setMethod(method, "")
       .setUrl(std::move(url));
    return req;
}

// ---- tests ----------------------------------------------------------------

TEST_CASE("Router - basic GET route matched") {
    FixedHandler handler(HTTPResponse::ok());
    Router router;
    router.GET("/health", &handler);

    auto req = makeRequest(HTTPMethod::GET, "/health");
    auto res = router.route(req);

    CHECK(res.statusCode() == 200);
}

TEST_CASE("Router - unknown path returns 404") {
    FixedHandler handler(HTTPResponse::ok());
    Router router;
    router.GET("/health", &handler);

    auto req = makeRequest(HTTPMethod::GET, "/unknown");
    auto res = router.route(req);

    CHECK(res.statusCode() == 404);
}

TEST_CASE("Router - known path with wrong method returns 405") {
    FixedHandler handler(HTTPResponse::ok());
    Router router;
    router.GET("/items", &handler);

    auto req = makeRequest(HTTPMethod::POST, "/items");
    auto res = router.route(req);

    CHECK(res.statusCode() == 405);
}

TEST_CASE("Router - path parameter is extracted and injected") {
    CapturingHandler handler;
    Router router;
    router.GET("/users/{id}", &handler);

    auto req = makeRequest(HTTPMethod::GET, "/users/42");
    [[maybe_unused]] auto res = router.route(req);

    REQUIRE(handler.last_request.has_value());
    auto id = handler.last_request->pathParam("id");
    REQUIRE(id.has_value());
    CHECK(*id == "42");
}

TEST_CASE("Router - multiple path parameters are all extracted") {
    CapturingHandler handler;
    Router router;
    router.GET("/orgs/{org}/repos/{repo}", &handler);

    auto req = makeRequest(HTTPMethod::GET, "/orgs/acme/repos/core");
    [[maybe_unused]] auto res = router.route(req);

    REQUIRE(handler.last_request.has_value());
    CHECK(handler.last_request->pathParam("org").value()  == "acme");
    CHECK(handler.last_request->pathParam("repo").value() == "core");
}

TEST_CASE("Router - authorize() returning false yields 403") {
    ForbiddenHandler handler;
    Router router;
    router.GET("/secret", &handler);

    auto req = makeRequest(HTTPMethod::GET, "/secret");
    auto res = router.route(req);

    CHECK(res.statusCode() == 403);
}

TEST_CASE("Router - correct handler is selected among multiple routes") {
    FixedHandler users(HTTPResponse::ok());
    FixedHandler items(HTTPResponse::created());
    Router router;
    router.GET("/users", &users);
    router.GET("/items", &items);

    auto req = makeRequest(HTTPMethod::GET, "/items");
    auto res = router.route(req);

    CHECK(res.statusCode() == 201);
}

TEST_CASE("Router - POST, PUT, PATCH, DELETE convenience wrappers") {
    FixedHandler h(HTTPResponse::ok());
    Router router;
    router.POST   ("/a", &h);
    router.PUT    ("/b", &h);
    router.PATCH  ("/c", &h);
    router.DELETE_("/d", &h);

    auto ra = makeRequest(HTTPMethod::POST,   "/a");
    auto rb = makeRequest(HTTPMethod::PUT,    "/b");
    auto rc = makeRequest(HTTPMethod::PATCH,  "/c");
    auto rd = makeRequest(HTTPMethod::DELETE, "/d");

    CHECK(router.route(ra).statusCode() == 200);
    CHECK(router.route(rb).statusCode() == 200);
    CHECK(router.route(rc).statusCode() == 200);
    CHECK(router.route(rd).statusCode() == 200);
}

TEST_CASE("Router - same path registered for different methods") {
    FixedHandler get_handler(HTTPResponse::ok());
    FixedHandler post_handler(HTTPResponse::created());
    Router router;
    router.GET ("/resource", &get_handler);
    router.POST("/resource", &post_handler);

    auto get_req  = makeRequest(HTTPMethod::GET,  "/resource");
    auto post_req = makeRequest(HTTPMethod::POST, "/resource");

    CHECK(router.route(get_req).statusCode()  == 200);
    CHECK(router.route(post_req).statusCode() == 201);
}

TEST_CASE("Router - custom not found handler is called") {
    FixedHandler fallback(HTTPResponse(418));  // I'm a teapot
    Router router;
    router.setNotFoundHandler(&fallback);

    auto req = makeRequest(HTTPMethod::GET, "/nowhere");
    CHECK(router.route(req).statusCode() == 418);
}

TEST_CASE("Router - custom method not allowed handler is called") {
    FixedHandler get_handler(HTTPResponse::ok());
    FixedHandler mna_handler(HTTPResponse(405));
    mna_handler = FixedHandler(HTTPResponse::methodNotAllowed());
    Router router;
    router.GET("/data", &get_handler);
    router.setMethodNotAllowedHandler(&mna_handler);

    auto req = makeRequest(HTTPMethod::DELETE, "/data");
    CHECK(router.route(req).statusCode() == 405);
}

TEST_CASE("Router - handler is only called once per request") {
    CapturingHandler handler;
    Router router;
    router.GET("/ping", &handler);

    auto req = makeRequest(HTTPMethod::GET, "/ping");
    [[maybe_unused]] auto res = router.route(req);

    CHECK(handler.call_count == 1);
}

TEST_CASE("Router - path params not set on 405 response") {
    CapturingHandler handler;
    Router router;
    router.GET("/users/{id}", &handler);

    auto req = makeRequest(HTTPMethod::POST, "/users/99");
    [[maybe_unused]] auto res = router.route(req);

    CHECK(handler.call_count == 0);
    CHECK(req.pathParams().empty());
}
