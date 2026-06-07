#include "http_response.hpp"

#include <algorithm>
#include <cctype>

// ---- constructors ---------------------------------------------------------

HTTPResponse::HTTPResponse(const HTTPResponse& other)
    : status_code_(other.status_code_)
    , reason_(other.reason_)
    , version_(other.version_)
    , headers_(other.headers_)
    , body_storage_(other.body_storage_)
    , body_(body_storage_)              // point into OUR storage, not other's
{}

HTTPResponse::HTTPResponse(HTTPResponse&& other) noexcept
    : status_code_(other.status_code_)
    , reason_(std::move(other.reason_))
    , version_(std::move(other.version_))
    , headers_(std::move(other.headers_))
    , body_storage_(std::move(other.body_storage_))
    , body_(body_storage_)              // point into OUR storage
{
    other.body_ = {};
}

HTTPResponse& HTTPResponse::operator=(const HTTPResponse& other) {
    if (this != &other) {
        status_code_  = other.status_code_;
        reason_       = other.reason_;
        version_      = other.version_;
        headers_      = other.headers_;
        body_storage_ = other.body_storage_;
        body_         = body_storage_;
    }
    return *this;
}

HTTPResponse& HTTPResponse::operator=(HTTPResponse&& other) noexcept {
    if (this != &other) {
        status_code_  = other.status_code_;
        reason_       = std::move(other.reason_);
        version_      = std::move(other.version_);
        headers_      = std::move(other.headers_);
        body_storage_ = std::move(other.body_storage_);
        body_         = body_storage_;
        other.body_   = {};
    }
    return *this;
}

HTTPResponse::HTTPResponse(uint16_t status_code)
    : status_code_(status_code)
    , reason_(defaultReasonPhrase(status_code))
{}

// ---- factory helpers ------------------------------------------------------

HTTPResponse HTTPResponse::ok()                  { return HTTPResponse(200); }
HTTPResponse HTTPResponse::created()             { return HTTPResponse(201); }
HTTPResponse HTTPResponse::noContent()           { return HTTPResponse(204); }
HTTPResponse HTTPResponse::badRequest()          { return HTTPResponse(400); }
HTTPResponse HTTPResponse::unauthorized()        { return HTTPResponse(401); }
HTTPResponse HTTPResponse::forbidden()           { return HTTPResponse(403); }
HTTPResponse HTTPResponse::notFound()            { return HTTPResponse(404); }
HTTPResponse HTTPResponse::methodNotAllowed()    { return HTTPResponse(405); }
HTTPResponse HTTPResponse::internalServerError() { return HTTPResponse(500); }
HTTPResponse HTTPResponse::serviceUnavailable()  { return HTTPResponse(503); }

// ---- accessors ------------------------------------------------------------

std::string_view HTTPResponse::header(std::string_view name) const {
    std::string key(name);
    std::transform(key.begin(), key.end(), key.begin(),
        [](unsigned char c) { return std::tolower(c); });

    auto it = headers_.find(key);
    return (it != headers_.end()) ? std::string_view(it->second) : std::string_view{};
}

std::string_view HTTPResponse::bodyAsText() const noexcept {
    return { reinterpret_cast<const char*>(body_.data()), body_.size() };
}

// ---- setters --------------------------------------------------------------

HTTPResponse& HTTPResponse::setStatus(uint16_t code) {
    status_code_ = code;
    reason_      = defaultReasonPhrase(code);
    return *this;
}

HTTPResponse& HTTPResponse::setStatus(uint16_t code, std::string reason) {
    status_code_ = code;
    reason_      = std::move(reason);
    return *this;
}

HTTPResponse& HTTPResponse::setVersion(std::string v) {
    version_ = std::move(v);
    return *this;
}

HTTPResponse& HTTPResponse::addHeader(std::string name, std::string value) {
    std::transform(name.begin(), name.end(), name.begin(),
        [](unsigned char c) { return std::tolower(c); });
    headers_.insert_or_assign(std::move(name), std::move(value));
    return *this;
}

HTTPResponse& HTTPResponse::setBody(std::vector<std::byte> data) {
    body_storage_ = std::move(data);
    body_         = body_storage_;
    addHeader("content-length", std::to_string(body_storage_.size()));
    return *this;
}

HTTPResponse& HTTPResponse::setBody(std::string_view text, std::string_view content_type) {
    body_storage_.assign(
        reinterpret_cast<const std::byte*>(text.data()),
        reinterpret_cast<const std::byte*>(text.data() + text.size()));
    body_ = body_storage_;
    addHeader("content-type",   std::string(content_type));
    addHeader("content-length", std::to_string(body_storage_.size()));
    return *this;
}

HTTPResponse& HTTPResponse::setJsonBody(std::string_view json) {
    return setBody(json, "application/json");
}

// ---- static ---------------------------------------------------------------

std::string HTTPResponse::defaultReasonPhrase(uint16_t code) noexcept {
    switch (code) {
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 204: return "No Content";
        case 206: return "Partial Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 413: return "Content Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        case 422: return "Unprocessable Content";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        default:  return "Unknown";
    }
}
