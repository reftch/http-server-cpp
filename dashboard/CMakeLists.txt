file(COPY assets DESTINATION ${CMAKE_BINARY_DIR}/dashboard)

find_package(OpenSSL REQUIRED)

add_executable(dashboard src/main.cc)

target_link_libraries(dashboard
  PRIVATE
    http-server
    OpenSSL::SSL
    OpenSSL::Crypto
)
