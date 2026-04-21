cmake_minimum_required(VERSION 3.14)
project(http-server 
    VERSION 1.0.0 
    DESCRIPTION "Http server"
    HOMEPAGE_URL "https://github.com/reftch/http-server-cpp"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect all .cpp files recursively from src/
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(server ${SOURCES})
