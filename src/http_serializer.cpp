#include "http_serializer.hpp"

#include <sys/socket.h>

std::string HTTPSerializer::serialize(const HTTPResponse& response) {
    std::string out;
    out.reserve(256 + response.body().size());

    out += std::string(response.httpVersion());
    out += ' ';
    out += std::to_string(response.statusCode());
    out += ' ';
    out += std::string(response.reasonPhrase());
    out += "\r\n";

    for (const auto& [name, value] : response.headers())
        out += name + ": " + value + "\r\n";

    out += "\r\n";

    auto body = response.bodyAsText();
    out.append(body.data(), body.size());

    return out;
}

bool HTTPSerializer::send(int fd, const HTTPResponse& response) {
    const auto data = serialize(response);
    std::size_t sent = 0;
    while (sent < data.size()) {
        // MSG_NOSIGNAL prevents SIGPIPE if the client closed the connection.
        ssize_t n = ::send(fd,
                           data.data() + sent,
                           data.size() - sent,
                           MSG_NOSIGNAL);
        if (n <= 0) return false;
        sent += static_cast<std::size_t>(n);
    }
    return true;
}
