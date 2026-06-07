#include "http_parser.hpp"

#include <sys/socket.h>
#include <sstream>

std::optional<HTTPRequest> HTTPParser::parse(int fd) {
    auto first_line = readLine(fd);
    if (!first_line || first_line->empty())
        return std::nullopt;

    std::istringstream ss(*first_line);
    std::string method_str, url, version;
    ss >> method_str >> url >> version;
    if (method_str.empty() || url.empty() || version.empty())
        return std::nullopt;

    HTTPRequest req;
    req.setMethod(HTTPRequest::parseMethod(method_str), method_str)
       .setUrl(url)
       .setVersion(version);

    while (true) {
        auto line = readLine(fd);
        if (!line)          return std::nullopt;
        if (line->empty())  break;

        auto colon = line->find(':');
        if (colon == std::string::npos) continue;

        std::string name  = line->substr(0, colon);
        std::string value = line->substr(colon + 1);

        auto start = value.find_first_not_of(" \t");
        if (start != std::string::npos)
            value = value.substr(start);

        req.addHeader(std::move(name), std::move(value));
    }

    auto content_length = req.contentLength();
    if (content_length && *content_length > 0) {
        std::vector<std::byte> body;
        if (!readBytes(fd, body, *content_length))
            return std::nullopt;
        req.setBody(std::move(body));
    }

    return req;
}

std::optional<std::string> HTTPParser::readLine(int fd) {
    std::string line;
    char c;
    while (true) {
        ssize_t n = recv(fd, &c, 1, 0);
        if (n <= 0) return std::nullopt;
        if (c == '\r') continue;
        if (c == '\n') return line;
        line += c;
    }
}

bool HTTPParser::readBytes(int fd, std::vector<std::byte>& buf, std::size_t count) {
    buf.resize(count);
    std::size_t received = 0;
    while (received < count) {
        ssize_t n = recv(fd,
                         reinterpret_cast<char*>(buf.data()) + received,
                         count - received, 0);
        if (n <= 0) return false;
        received += static_cast<std::size_t>(n);
    }
    return true;
}
