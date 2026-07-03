find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")

add_executable(client src/main.cc)

target_link_libraries(client
  PRIVATE
    http-server
    OpenSSL::SSL
    OpenSSL::Crypto
)