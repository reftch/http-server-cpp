add_executable(client src/main.cc)
target_link_libraries(client http-server)
