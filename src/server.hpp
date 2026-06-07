#pragma once

#include "router.hpp"

#include <atomic>
#include <cstdint>
#include <string>

class Server {
public:
    Server(uint16_t port, Router router);
    ~Server();

    // Blocks until stop() is called (from another thread or signal handler).
    void run();
    void stop();

    [[nodiscard]] uint16_t port()    const noexcept { return port_; }
    [[nodiscard]] bool     running() const noexcept { return running_; }

private:
    // Entry point for each per-connection thread.
    // Owns its own Router copy and the client socket.
    static void handleConnection(int          client_fd,
                                 Router       router,
                                 std::string  remote_addr,
                                 uint16_t     remote_port);

    uint16_t          port_;
    Router            router_;
    int               server_fd_ { -1 };
    std::atomic<bool> running_   { false };
};
