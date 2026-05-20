file(COPY assets DESTINATION ${CMAKE_BINARY_DIR}/examples/server)

add_executable(server src/main.cc)
target_link_libraries(server http-server)
