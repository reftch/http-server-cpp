cmake_minimum_required(VERSION 3.14)
project(http-server 
    VERSION 1.0.0 
    DESCRIPTION "Http server"
    HOMEPAGE_URL "https://github.com/reftch/http-server-cpp"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type for single-config generators
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, defaulting to Debug")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING 
        "Choose the type of build (Debug, Release, RelWithDebInfo, MinSizeRel)" 
        FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "CXX Standard: ${CMAKE_CXX_STANDARD}")

# Collect all .cpp files recursively from src/
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(server ${SOURCES})

# Output to build/bin for better organization
set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/build/debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/build/release"
)

# Custom targets
add_custom_target(run
    COMMAND "${CMAKE_BINARY_DIR}/build/$<CONFIG>/server"
    DEPENDS debug
    DEPENDS server
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Running server"
)

add_custom_target(debug
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug .
)

add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E remove_directory build CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove build CMakeCache.txt cmake_install.cmake Makefile
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Removing all build artifacts"
    VERBATIM
)

add_custom_target(release
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release .
    COMMAND ${CMAKE_COMMAND} --build .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Building optimized Release version"
)