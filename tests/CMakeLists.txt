add_executable(test_http_request test_http_request.cpp)
target_link_libraries(test_http_request PRIVATE dean doctest::doctest)
target_include_directories(test_http_request PRIVATE ${doctest_SOURCE_DIR}/doctest)
add_test(NAME http_request_tests COMMAND test_http_request)

add_executable(test_router test_router.cpp)
target_link_libraries(test_router PRIVATE dean doctest::doctest)
target_include_directories(test_router PRIVATE ${doctest_SOURCE_DIR}/doctest)
add_test(NAME router_tests COMMAND test_router)
