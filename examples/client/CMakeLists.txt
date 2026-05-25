find_package(OpenSSL REQUIRED)
add_compile_definitions(USE_SSL)

add_executable(client src/main.cc)

# target_link_libraries(client http-server)
target_link_libraries(client
  PRIVATE
    http-server
    OpenSSL::SSL
    OpenSSL::Crypto
)