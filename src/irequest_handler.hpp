#pragma once

#include "http_request.hpp"
#include "http_response.hpp"

class IRequestHandler {
public:
    virtual ~IRequestHandler() = default;

    [[nodiscard]] virtual HTTPResponse handle(const HTTPRequest& request) = 0;

    // Optional hook called before handle(). Return false to abort and let the
    // router respond with 403. Default implementation always allows.
    [[nodiscard]] virtual bool authorize(const HTTPRequest&) { return true; }
};
