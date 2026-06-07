#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

enum class HTTPMethod {
    GET, POST, PUT, PATCH, DELETE, HEAD, OPTIONS, TRACE, CONNECT, UNKNOWN
};

class HTTPRequest {
public:
    using Headers     = std::unordered_map<std::string, std::string>;
    using QueryParams = std::unordered_map<std::string, std::string>;
    using PathParams  = std::unordered_map<std::string, std::string>;

    HTTPRequest() = default;
    HTTPRequest(const HTTPRequest& other);
    HTTPRequest(HTTPRequest&& other) noexcept;
    HTTPRequest& operator=(const HTTPRequest& other);
    HTTPRequest& operator=(HTTPRequest&& other) noexcept;
    ~HTTPRequest() = default;

    // ---- accessors --------------------------------------------------------

    [[nodiscard]] HTTPMethod        method()       const noexcept { return method_; }
    [[nodiscard]] std::string_view  methodString() const noexcept { return method_str_; }
    [[nodiscard]] std::string_view  path()         const noexcept { return path_; }
    [[nodiscard]] std::string_view  rawUrl()       const noexcept { return raw_url_; }
    [[nodiscard]] std::string_view  httpVersion()  const noexcept { return version_; }
    [[nodiscard]] std::string_view  remoteAddr()   const noexcept { return remote_addr_; }
    [[nodiscard]] uint16_t          remotePort()   const noexcept { return remote_port_; }
    [[nodiscard]] bool              isSecure()     const noexcept { return is_secure_; }

    [[nodiscard]] const Headers&     headers()     const noexcept { return headers_; }
    [[nodiscard]] const QueryParams& queryParams() const noexcept { return query_params_; }
    [[nodiscard]] const PathParams&  pathParams()  const noexcept { return path_params_; }

    [[nodiscard]] std::optional<std::string_view> header(std::string_view name)     const;
    [[nodiscard]] std::optional<std::string_view> queryParam(std::string_view key)  const;
    [[nodiscard]] std::optional<std::string_view> pathParam(std::string_view key)   const;

    [[nodiscard]] std::span<const std::byte>  body()       const noexcept { return body_; }
    [[nodiscard]] std::string_view            bodyAsText() const noexcept;

    [[nodiscard]] std::optional<std::string_view> contentType()   const;
    [[nodiscard]] std::optional<std::size_t>      contentLength() const;

    [[nodiscard]] bool isMethod(HTTPMethod m)  const noexcept { return method_ == m; }
    [[nodiscard]] bool hasBody()               const noexcept { return !body_.empty(); }
    [[nodiscard]] bool expectsContinue()       const;

    // ---- builder-style setters (for use by the server / parser) -----------

    HTTPRequest& setMethod(HTTPMethod m, std::string raw);
    HTTPRequest& setUrl(std::string url);
    HTTPRequest& setVersion(std::string v);
    HTTPRequest& setRemote(std::string addr, uint16_t port);
    HTTPRequest& setSecure(bool s) noexcept { is_secure_ = s; return *this; }
    HTTPRequest& addHeader(std::string name, std::string value);
    HTTPRequest& setPathParams(PathParams params);
    HTTPRequest& setBody(std::vector<std::byte> data);
    HTTPRequest& setBody(std::string_view text);

    static HTTPMethod parseMethod(std::string_view s) noexcept;

private:
    HTTPMethod   method_      { HTTPMethod::UNKNOWN };
    std::string  method_str_;
    std::string  raw_url_;
    std::string  path_;
    std::string  version_;
    std::string  remote_addr_;
    uint16_t     remote_port_ { 0 };
    bool         is_secure_   { false };

    Headers      headers_;
    QueryParams  query_params_;
    PathParams   path_params_;

    std::vector<std::byte> body_storage_;
    std::span<const std::byte> body_;
};
