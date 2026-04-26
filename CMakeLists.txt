cmake_minimum_required(VERSION 3.14)
project(http-server
    VERSION 1.0.0
    DESCRIPTION "Http server"
    HOMEPAGE_URL "https://github.com/reftch/http-server-cpp"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include the header files
include_directories(${PROJECT_SOURCE_DIR}/src)

# Collect all .cpp files recursively from src/
# file(GLOB_RECURSE SOURCES "src/*.cpp")

# Sources DIR
# add_executable(server ${SOURCES})

add_subdirectory(src)

enable_testing() 
add_subdirectory(test)

# Add optimization and static linking to match your g++ command
# target_compile_options(server PRIVATE -O3)

# macOS static libc++ (most redistributable option)
# if(APPLE)
#     target_link_options(server PRIVATE
#         -Wl,-dead_strip # Remove unused code
#     )
# else()
#     target_link_options(server PRIVATE -static)
# endif()
