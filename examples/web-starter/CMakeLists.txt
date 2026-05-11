file(COPY static DESTINATION ${CMAKE_BINARY_DIR}/examples/web-starter)

add_executable(web-starter main.cpp)
target_link_libraries(web-starter http-server)