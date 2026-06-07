#include "router.hpp"

// ---- route registration ---------------------------------------------------

Router& Router::addRoute(HTTPMethod method, std::string pattern, IRequestHandler* handler) {
    routes_.push_back({
        method,
        pattern,
        splitPath(pattern),
        handler
    });
    return *this;
}

Router& Router::setNotFoundHandler(IRequestHandler* handler) {
    not_found_handler_ = handler;
    return *this;
}

Router& Router::setMethodNotAllowedHandler(IRequestHandler* handler) {
    method_not_allowed_handler_ = handler;
    return *this;
}

// ---- dispatch -------------------------------------------------------------

HTTPResponse Router::route(HTTPRequest& request) const {
    const auto path_segments = splitPath(request.path());

    bool path_matched = false;

    for (const auto& route : routes_) {
        HTTPRequest::PathParams params;

        if (!matchSegments(route.segments, path_segments, params))
            continue;

        path_matched = true;

        if (route.method != request.method())
            continue;

        request.setPathParams(std::move(params));

        if (!route.handler->authorize(request))
            return HTTPResponse::forbidden();

        return route.handler->handle(request);
    }

    if (path_matched) {
        if (method_not_allowed_handler_)
            return method_not_allowed_handler_->handle(request);
        return HTTPResponse::methodNotAllowed();
    }

    if (not_found_handler_)
        return not_found_handler_->handle(request);
    return HTTPResponse::notFound();
}

// ---- private helpers ------------------------------------------------------

std::vector<std::string> Router::splitPath(std::string_view path) {
    std::vector<std::string> segments;
    std::string segment;

    for (char c : path) {
        if (c == '/') {
            if (!segment.empty()) {
                segments.push_back(std::move(segment));
                segment.clear();
            }
        } else {
            segment += c;
        }
    }

    if (!segment.empty())
        segments.push_back(std::move(segment));

    return segments;
}

bool Router::matchSegments(const std::vector<std::string>& pattern,
                           const std::vector<std::string>& path,
                           HTTPRequest::PathParams&         params) {
    if (pattern.size() != path.size())
        return false;

    HTTPRequest::PathParams captured;

    for (std::size_t i = 0; i < pattern.size(); ++i) {
        const auto& seg = pattern[i];

        if (seg.size() >= 2 && seg.front() == '{' && seg.back() == '}') {
            captured[seg.substr(1, seg.size() - 2)] = path[i];
        } else if (seg != path[i]) {
            return false;
        }
    }

    params = std::move(captured);
    return true;
}
