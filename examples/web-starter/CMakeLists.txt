file(COPY assets DESTINATION ${CMAKE_BINARY_DIR}/examples/web-starter)

add_executable(web-starter src/main.cc)
target_link_libraries(web-starter http-server)