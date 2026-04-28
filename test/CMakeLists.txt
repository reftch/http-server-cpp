
add_executable (web_app web_app.cpp)
target_link_libraries(web_app http-server)

add_executable (tests server_test.cpp)
# target_link_libraries(server_test http-server)

# Link:
# - your library (http-server)
# - GoogleTest (gtest_main gives you main(); gtest for APIs)
target_link_libraries(tests PRIVATE
    http-server
    GTest::gtest
    GTest::gtest_main
)

# Optional: register with CTest so `ctest` runs it
# include(GoogleTest)
# gtest_discover_tests(server_test)

