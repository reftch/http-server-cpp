# copy static resources
file(COPY static DESTINATION ${CMAKE_BINARY_DIR}/test)

add_executable (web_app web_app.cpp)
target_link_libraries(web_app http-server)

# Create a combined tests executable
add_executable (tests
    server_test.cpp
    router_test.cpp
)

# Add compile definitions to distinguish between test types
target_compile_definitions(tests PRIVATE
    SERVER_TEST
    ROUTER_TEST
)

target_link_libraries(tests PRIVATE
    http-server
    GTest::gtest
    GTest::gtest_main
)