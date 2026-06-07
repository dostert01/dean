#include "http_request.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

// ---- copy / move ----------------------------------------------------------

HTTPRequest::HTTPRequest(const HTTPRequest& other)
    : method_(other.method_)
    , method_str_(other.method_str_)
    , raw_url_(other.raw_url_)
    , path_(other.path_)
    , version_(other.version_)
    , remote_addr_(other.remote_addr_)
    , remote_port_(other.remote_port_)
    , is_secure_(other.is_secure_)
    , headers_(other.headers_)
    , query_params_(other.query_params_)
    , path_params_(other.path_params_)
    , body_storage_(other.body_storage_)
    , body_(body_storage_)              // point into OUR storage, not other's
{}

HTTPRequest::HTTPRequest(HTTPRequest&& other) noexcept
    : method_(other.method_)
    , method_str_(std::move(other.method_str_))
    , raw_url_(std::move(other.raw_url_))
    , path_(std::move(other.path_))
    , version_(std::move(other.version_))
    , remote_addr_(std::move(other.remote_addr_))
    , remote_port_(other.remote_port_)
    , is_secure_(other.is_secure_)
    , headers_(std::move(other.headers_))
    , query_params_(std::move(other.query_params_))
    , path_params_(std::move(other.path_params_))
    , body_storage_(std::move(other.body_storage_))
    , body_(body_storage_)              // point into OUR storage
{
    other.body_ = {};
}

HTTPRequest& HTTPRequest::operator=(const HTTPRequest& other) {
    if (this != &other) {
        method_       = other.method_;
        method_str_   = other.method_str_;
        raw_url_      = other.raw_url_;
        path_         = other.path_;
        version_      = other.version_;
        remote_addr_  = other.remote_addr_;
        remote_port_  = other.remote_port_;
        is_secure_    = other.is_secure_;
        headers_      = other.headers_;
        query_params_ = other.query_params_;
        path_params_  = other.path_params_;
        body_storage_ = other.body_storage_;
        body_         = body_storage_;
    }
    return *this;
}

HTTPRequest& HTTPRequest::operator=(HTTPRequest&& other) noexcept {
    if (this != &other) {
        method_       = other.method_;
        method_str_   = std::move(other.method_str_);
        raw_url_      = std::move(other.raw_url_);
        path_         = std::move(other.path_);
        version_      = std::move(other.version_);
        remote_addr_  = std::move(other.remote_addr_);
        remote_port_  = other.remote_port_;
        is_secure_    = other.is_secure_;
        headers_      = std::move(other.headers_);
        query_params_ = std::move(other.query_params_);
        path_params_  = std::move(other.path_params_);
        body_storage_ = std::move(other.body_storage_);
        body_         = body_storage_;
        other.body_   = {};
    }
    return *this;
}

// ---- static helper --------------------------------------------------------

HTTPMethod HTTPRequest::parseMethod(std::string_view s) noexcept {
    if (s == "GET")     return HTTPMethod::GET;
    if (s == "POST")    return HTTPMethod::POST;
    if (s == "PUT")     return HTTPMethod::PUT;
    if (s == "PATCH")   return HTTPMethod::PATCH;
    if (s == "DELETE")  return HTTPMethod::DELETE;
    if (s == "HEAD")    return HTTPMethod::HEAD;
    if (s == "OPTIONS") return HTTPMethod::OPTIONS;
    if (s == "TRACE")   return HTTPMethod::TRACE;
    if (s == "CONNECT") return HTTPMethod::CONNECT;
    return HTTPMethod::UNKNOWN;
}

// ---- accessors ------------------------------------------------------------

std::optional<std::string_view> HTTPRequest::header(std::string_view name) const {
    std::string key(name);
    std::transform(key.begin(), key.end(), key.begin(),
        [](unsigned char c) { return std::tolower(c); });

    auto it = headers_.find(key);
    if (it == headers_.end()) return std::nullopt;
    return std::string_view(it->second);
}

std::optional<std::string_view> HTTPRequest::queryParam(std::string_view key) const {
    auto it = query_params_.find(std::string(key));
    if (it == query_params_.end()) return std::nullopt;
    return std::string_view(it->second);
}

std::optional<std::string_view> HTTPRequest::pathParam(std::string_view key) const {
    auto it = path_params_.find(std::string(key));
    if (it == path_params_.end()) return std::nullopt;
    return std::string_view(it->second);
}

std::string_view HTTPRequest::bodyAsText() const noexcept {
    return { reinterpret_cast<const char*>(body_.data()), body_.size() };
}

std::optional<std::string_view> HTTPRequest::contentType() const {
    return header("content-type");
}

std::optional<std::size_t> HTTPRequest::contentLength() const {
    auto v = header("content-length");
    if (!v) return std::nullopt;
    try {
        return std::stoul(std::string(*v));
    } catch (...) {
        return std::nullopt;
    }
}

bool HTTPRequest::expectsContinue() const {
    auto v = header("expect");
    return v && *v == "100-continue";
}

// ---- setters --------------------------------------------------------------

HTTPRequest& HTTPRequest::setMethod(HTTPMethod m, std::string raw) {
    method_     = m;
    method_str_ = std::move(raw);
    return *this;
}

HTTPRequest& HTTPRequest::setUrl(std::string url) {
    raw_url_ = std::move(url);

    // Split path and query string at the first '?'
    auto qpos = raw_url_.find('?');
    if (qpos == std::string::npos) {
        path_ = raw_url_;
        query_params_.clear();
        return *this;
    }

    path_ = raw_url_.substr(0, qpos);

    // Parse key=value pairs separated by '&'
    std::string_view qs(raw_url_.data() + qpos + 1, raw_url_.size() - qpos - 1);
    while (!qs.empty()) {
        auto amp = qs.find('&');
        auto pair = (amp == std::string_view::npos) ? qs : qs.substr(0, amp);

        auto eq = pair.find('=');
        if (eq != std::string_view::npos) {
            query_params_.insert_or_assign(
                std::string(pair.substr(0, eq)),
                std::string(pair.substr(eq + 1)));
        } else if (!pair.empty()) {
            query_params_.insert_or_assign(std::string(pair), "");
        }

        qs = (amp == std::string_view::npos) ? std::string_view{} : qs.substr(amp + 1);
    }

    return *this;
}

HTTPRequest& HTTPRequest::setVersion(std::string v) {
    version_ = std::move(v);
    return *this;
}

HTTPRequest& HTTPRequest::setRemote(std::string addr, uint16_t port) {
    remote_addr_ = std::move(addr);
    remote_port_ = port;
    return *this;
}

HTTPRequest& HTTPRequest::setPathParams(PathParams params) {
    path_params_ = std::move(params);
    return *this;
}

HTTPRequest& HTTPRequest::addHeader(std::string name, std::string value) {
    std::transform(name.begin(), name.end(), name.begin(),
        [](unsigned char c) { return std::tolower(c); });
    headers_.insert_or_assign(std::move(name), std::move(value));
    return *this;
}

HTTPRequest& HTTPRequest::setBody(std::vector<std::byte> data) {
    body_storage_ = std::move(data);
    body_         = body_storage_;
    return *this;
}

HTTPRequest& HTTPRequest::setBody(std::string_view text) {
    body_storage_.assign(
        reinterpret_cast<const std::byte*>(text.data()),
        reinterpret_cast<const std::byte*>(text.data() + text.size()));
    body_ = body_storage_;
    return *this;
}
