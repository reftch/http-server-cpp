set(TARGET_NAME rest)
# find_package(OpenSSL REQUIRED)
# message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")

# set(OPENSSL_USE_STATIC_LIBS TRUE)
# find_package(OpenSSL REQUIRED)

add_executable(${TARGET_NAME} src/main.cc)

target_link_libraries(${TARGET_NAME}
  PRIVATE
    http-server
    # OpenSSL::SSL
    # OpenSSL::Crypto
)

# -----------------------------
# SIZE OPTIMIZATION
# -----------------------------
target_compile_options(${TARGET_NAME} PRIVATE
    -Os
    -ffunction-sections
    -fdata-sections
    -fno-exceptions
    -fno-rtti
    -fvisibility=hidden
    -fno-unwind-tables
    -fno-asynchronous-unwind-tables
)

if(NOT APPLE)
  target_link_options(${TARGET_NAME} PRIVATE
    -Wl,--gc-sections,-s
)
else()
  target_link_options(${TARGET_NAME} PRIVATE
    -Wl,-dead_strip
)
endif()

# -----------------------------
# LTO
# -----------------------------
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
