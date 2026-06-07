#pragma once

#include "http_response.hpp"
#include <string>

class HTTPSerializer {
public:
    [[nodiscard]] static std::string serialize(const HTTPResponse& response);

    // Writes the serialized response to fd. Returns false on send error.
    static bool send(int fd, const HTTPResponse& response);
};
