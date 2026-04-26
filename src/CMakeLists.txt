add_library(server
    header.hpp
    request.cpp
    request.hpp
    response.hpp
    response.cpp
    server.cpp
    server.hpp
    utils.cpp
    utils.hpp
)

add_executable(http-server main.cpp)
target_link_libraries(http-server server)
