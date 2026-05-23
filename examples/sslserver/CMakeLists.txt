find_package(OpenSSL REQUIRED)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR}/examples/sslserver)

add_executable(sslserver src/main.cc)

target_link_libraries(sslserver
  PRIVATE
    http-server
    OpenSSL::SSL
    OpenSSL::Crypto
)