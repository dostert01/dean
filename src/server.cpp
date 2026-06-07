#include "server.hpp"
#include "http_parser.hpp"
#include "http_serializer.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdexcept>
#include <thread>

Server::Server(uint16_t port, Router router)
    : port_(port)
    , router_(std::move(router))
{}

Server::~Server() {
    stop();
}

void Server::run() {
    server_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0)
        throw std::runtime_error("socket() failed");

    int opt = 1;
    ::setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port_);

    if (::bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed on port " + std::to_string(port_));

    if (::listen(server_fd_, SOMAXCONN) < 0)
        throw std::runtime_error("listen() failed");

    running_ = true;

    while (running_) {
        sockaddr_in client_addr{};
        socklen_t   client_len = sizeof(client_addr);

        int client_fd = ::accept(server_fd_,
                                 reinterpret_cast<sockaddr*>(&client_addr),
                                 &client_len);
        if (client_fd < 0) {
            if (!running_) break;  // shutdown() unblocked accept()
            continue;
        }

        char ip_buf[INET_ADDRSTRLEN] = {};
        ::inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, sizeof(ip_buf));
        uint16_t client_port = ntohs(client_addr.sin_port);

        // Copy the template router into the new thread.
        // The thread owns both the router copy and the client socket.
        std::thread(
            &Server::handleConnection,
            client_fd,
            router_,                  // copy happens here
            std::string(ip_buf),
            client_port
        ).detach();
    }

    ::close(server_fd_);
    server_fd_ = -1;
}

void Server::stop() {
    running_ = false;
    if (server_fd_ >= 0)
        ::shutdown(server_fd_, SHUT_RDWR);
}

void Server::handleConnection(int         client_fd,
                               Router      router,
                               std::string remote_addr,
                               uint16_t    remote_port) {
    // Keep reading requests on this connection until the client closes it
    // or signals Connection: close (HTTP/1.1 persistent connections).
    while (true) {
        auto request = HTTPParser::parse(client_fd);
        if (!request) break;

        request->setRemote(remote_addr, remote_port);

        bool keep_alive = (request->httpVersion() != "HTTP/1.0");
        if (auto conn = request->header("connection"))
            keep_alive = (*conn != "close");

        HTTPResponse response = router.route(*request);

        if (!keep_alive)
            response.addHeader("connection", "close");

        if (!HTTPSerializer::send(client_fd, response)) break;
        if (!keep_alive) break;
    }

    ::close(client_fd);
}
