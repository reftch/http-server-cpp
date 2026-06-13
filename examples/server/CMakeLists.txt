file(COPY assets DESTINATION ${CMAKE_BINARY_DIR}/examples/server)
file(COPY assets2 DESTINATION ${CMAKE_BINARY_DIR}/examples/server)

find_package(OpenSSL REQUIRED)

add_executable(server src/main.cc)
target_link_libraries(server
  PRIVATE
    http-server
    OpenSSL::SSL
    OpenSSL::Crypto
)
