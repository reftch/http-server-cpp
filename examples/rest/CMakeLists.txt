find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")

add_executable(rest src/main.cc)

target_link_libraries(rest
  PRIVATE
    http-server
    OpenSSL::SSL
    OpenSSL::Crypto
)