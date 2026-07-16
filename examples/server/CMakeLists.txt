file(COPY assets DESTINATION ${CMAKE_BINARY_DIR}/examples/server)

find_package(OpenSSL REQUIRED)

add_executable(server src/main.cc)

include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)

FetchContent_MakeAvailable(json)

target_link_libraries(server
  PRIVATE
    http-server
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)
