find_package(OpenSSL REQUIRED)

add_compile_definitions(POLL_TIMEOUT=100)

# Create a combined tests executable
add_executable (tests
    logger_test.cc
    server_test.cc
    router_test.cc
    request_test.cc
    getenv_test.cc
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
    OpenSSL::SSL
    OpenSSL::Crypto
)
