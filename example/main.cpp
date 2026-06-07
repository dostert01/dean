#include "server.hpp"
#include "irequest_handler.hpp"

#include <csignal>
#include <iostream>
#include <pthread.h>
#include <thread>

static const std::string HELLO_HTML =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "  <head><title>Hello World</title></head>\n"
    "  <body><h1>Hello World</h1></body>\n"
    "</html>\n";

static const int port = 8888;
static const std::string route = "/helloworld";

class HelloWorldHandler : public IRequestHandler {
public:
    HTTPResponse handle(const HTTPRequest&) override {
        return HTTPResponse::ok().setBody(HELLO_HTML, "text/html; charset=utf-8");
    }
};

int main() {
    HelloWorldHandler hello;

    Router router;
    router.GET(route, &hello);

    Server server(port, std::move(router));

    // Block SIGTERM and SIGINT in all threads so sigwait() in main catches them.
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, nullptr);

    std::thread server_thread([&server]() {
        server.run();
    });

    std::cout << "Server listening on port 8888.\nConnect your browser to http://127.0.0.1:" //
        << port << route << " \nPress Ctrl+C to stop.\n";

    // Park the main thread waiting for a termination signal.
    int sig = 0;
    sigwait(&sigset, &sig);

    std::cout << "\nReceived signal " << sig << " — shutting down...\n";
    server.stop();
    server_thread.join();

    std::cout << "Goodbye.\n";
    return 0;
}
