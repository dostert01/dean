add_library(dean STATIC
    http_request.cpp
    http_request.hpp
    http_response.cpp
    http_response.hpp
    router.cpp
    router.hpp
    irequest_handler.hpp
    http_parser.cpp
    http_parser.hpp
    http_serializer.cpp
    http_serializer.hpp
    server.cpp
    server.hpp
)

find_package(Threads REQUIRED)

target_include_directories(dean PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dean PUBLIC Threads::Threads)
