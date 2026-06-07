#pragma once

#include "http_request.hpp"
#include "http_response.hpp"
#include "irequest_handler.hpp"

#include <string>
#include <vector>

class Router {
public:
    // ---- route registration -----------------------------------------------

    Router& addRoute(HTTPMethod method, std::string pattern, IRequestHandler* handler);

    // Convenience wrappers
    Router& GET    (std::string pattern, IRequestHandler* h) { return addRoute(HTTPMethod::GET,    std::move(pattern), h); }
    Router& POST   (std::string pattern, IRequestHandler* h) { return addRoute(HTTPMethod::POST,   std::move(pattern), h); }
    Router& PUT    (std::string pattern, IRequestHandler* h) { return addRoute(HTTPMethod::PUT,    std::move(pattern), h); }
    Router& PATCH  (std::string pattern, IRequestHandler* h) { return addRoute(HTTPMethod::PATCH,  std::move(pattern), h); }
    Router& DELETE_(std::string pattern, IRequestHandler* h) { return addRoute(HTTPMethod::DELETE, std::move(pattern), h); }

    // ---- fallback handlers (optional — built-in defaults used otherwise) --

    Router& setNotFoundHandler(IRequestHandler* handler);
    Router& setMethodNotAllowedHandler(IRequestHandler* handler);

    // ---- dispatch ---------------------------------------------------------

    [[nodiscard]] HTTPResponse route(HTTPRequest& request) const;

private:
    struct Route {
        HTTPMethod               method;
        std::string              pattern;
        std::vector<std::string> segments;
        IRequestHandler*         handler { nullptr };
    };

    static std::vector<std::string> splitPath(std::string_view path);

    static bool matchSegments(const std::vector<std::string>& pattern,
                              const std::vector<std::string>& path,
                              HTTPRequest::PathParams&         params);

    std::vector<Route> routes_;

    IRequestHandler* not_found_handler_            { nullptr };
    IRequestHandler* method_not_allowed_handler_   { nullptr };
};
