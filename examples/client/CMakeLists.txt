find_package(OpenSSL REQUIRED)

add_executable(client src/main.cc)

target_link_libraries(client
  PRIVATE
    http-server
    OpenSSL::SSL
    OpenSSL::Crypto
)