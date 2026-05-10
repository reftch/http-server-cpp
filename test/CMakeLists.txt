# copy static resources
file(COPY ../static DESTINATION ${CMAKE_BINARY_DIR}/test)

add_executable (web_app web_app.cpp)
target_link_libraries(web_app http-server)

add_executable (server server_test.cpp)
add_executable (router router_test.cpp)

# Link:
target_link_libraries(server PRIVATE
    http-server
    GTest::gtest
    GTest::gtest_main
)

target_link_libraries(router PRIVATE
    http-server
    GTest::gtest
    GTest::gtest_main
)
