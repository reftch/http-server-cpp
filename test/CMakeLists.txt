# Create a combined tests executable
add_executable (tests
    server_test.cpp
    router_test.cpp
    request_test.cpp
)

# Add compile definitions to distinguish between test types
target_compile_definitions(tests PRIVATE
    SERVER_TEST
    ROUTER_TEST
    REQUEST_TEST
)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
)
FetchContent_MakeAvailable(googletest)

target_link_libraries(tests PRIVATE
    http-server
    GTest::gtest
    GTest::gtest_main
)
