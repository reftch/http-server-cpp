cmake_minimum_required(VERSION 3.14)
project(http-server
    VERSION 1.0.0
    DESCRIPTION "Http server"
    HOMEPAGE_URL "https://github.com/reftch/http-server-cpp"
    LANGUAGES CXX)

# User-configurable options
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_TESTS "Build example applications" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add warn + opt flags globally
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

# Configuration
add_compile_definitions(KEEPALIVE_MAX_COUNT=96)
add_compile_definitions(CONNECTION_TIMEOUT_SECOND=300)
add_compile_definitions(READ_BUFFER_SIZE=4096)
add_compile_definitions(CLIENT_TIMEOUT_SECONDS=10)

# Enable clang-tidy performance checks
option(ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)
if(ENABLE_CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY
        clang-tidy;
        -checks=performance-unnecessary-value-param,
                performance-unnecessary-copy-initialization,
                performance-unnecessary-move,
                performance-inefficient-algorithm
    )
endif()

# Include the header files
include_directories(${PROJECT_SOURCE_DIR}/src)
add_subdirectory(src)
    
# Tests
if(BUILD_TESTS)
    enable_testing() 
    add_subdirectory(test)
endif()

if(BUILD_EXAMPLES) 
    add_subdirectory(examples)
endif()
