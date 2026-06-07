#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class HTTPResponse {
public:
    using Headers = std::unordered_map<std::string, std::string>;

    HTTPResponse() = default;
    explicit HTTPResponse(uint16_t status_code);
    HTTPResponse(const HTTPResponse& other);
    HTTPResponse(HTTPResponse&& other) noexcept;
    HTTPResponse& operator=(const HTTPResponse& other);
    HTTPResponse& operator=(HTTPResponse&& other) noexcept;
    ~HTTPResponse() = default;

    // ---- factory helpers --------------------------------------------------

    static HTTPResponse ok();
    static HTTPResponse created();
    static HTTPResponse noContent();
    static HTTPResponse badRequest();
    static HTTPResponse unauthorized();
    static HTTPResponse forbidden();
    static HTTPResponse notFound();
    static HTTPResponse methodNotAllowed();
    static HTTPResponse internalServerError();
    static HTTPResponse serviceUnavailable();

    // ---- accessors --------------------------------------------------------

    [[nodiscard]] uint16_t         statusCode()   const noexcept { return status_code_; }
    [[nodiscard]] std::string_view reasonPhrase() const noexcept { return reason_; }
    [[nodiscard]] std::string_view httpVersion()  const noexcept { return version_; }

    [[nodiscard]] const Headers& headers() const noexcept { return headers_; }

    [[nodiscard]] std::string_view header(std::string_view name) const;  // "" if absent

    [[nodiscard]] std::span<const std::byte> body()       const noexcept { return body_; }
    [[nodiscard]] std::string_view           bodyAsText() const noexcept;

    [[nodiscard]] bool isInformational() const noexcept { return status_code_ >= 100 && status_code_ < 200; }
    [[nodiscard]] bool isSuccess()       const noexcept { return status_code_ >= 200 && status_code_ < 300; }
    [[nodiscard]] bool isRedirection()   const noexcept { return status_code_ >= 300 && status_code_ < 400; }
    [[nodiscard]] bool isClientError()   const noexcept { return status_code_ >= 400 && status_code_ < 500; }
    [[nodiscard]] bool isServerError()   const noexcept { return status_code_ >= 500 && status_code_ < 600; }
    [[nodiscard]] bool isError()         const noexcept { return status_code_ >= 400; }

    // ---- builder-style setters --------------------------------------------

    HTTPResponse& setStatus(uint16_t code);
    HTTPResponse& setStatus(uint16_t code, std::string reason);
    HTTPResponse& setVersion(std::string v);
    HTTPResponse& addHeader(std::string name, std::string value);
    HTTPResponse& setBody(std::vector<std::byte> data);
    HTTPResponse& setBody(std::string_view text, std::string_view content_type = "text/plain");
    HTTPResponse& setJsonBody(std::string_view json);

    static std::string defaultReasonPhrase(uint16_t code) noexcept;

private:
    uint16_t    status_code_ { 200 };
    std::string reason_      { "OK" };
    std::string version_     { "HTTP/1.1" };

    Headers headers_;

    std::vector<std::byte>     body_storage_;
    std::span<const std::byte> body_;
};
