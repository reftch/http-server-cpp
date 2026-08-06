# find_package(OpenSSL REQUIRED)
# message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")

# set(OPENSSL_USE_STATIC_LIBS TRUE)
# find_package(OpenSSL REQUIRED)

add_executable(rest src/main.cc)

target_link_libraries(rest
  PRIVATE
    http-server
    # OpenSSL::SSL
    # OpenSSL::Crypto
)

# -----------------------------
# SIZE OPTIMIZATION
# -----------------------------
target_compile_options(rest PRIVATE
    -Os
    -ffunction-sections
    -fdata-sections
    -fno-exceptions
    -fno-rtti
)

if(NOT APPLE)
  target_link_options(rest PRIVATE
    -Wl,--gc-sections,-s
)
else()
  target_link_options(rest PRIVATE
    -Wl,-dead_strip
)
endif()

# -----------------------------
# LTO
# -----------------------------
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
