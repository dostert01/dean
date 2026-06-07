#pragma once

#include "http_request.hpp"
#include <optional>

class HTTPParser {
public:
    // Reads and parses one HTTP/1.1 request from a connected socket fd.
    // Returns nullopt on connection close or malformed input.
    static std::optional<HTTPRequest> parse(int fd);

private:
    static std::optional<std::string> readLine(int fd);
    static bool readBytes(int fd, std::vector<std::byte>& buf, std::size_t count);
};
