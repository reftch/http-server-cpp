# http-server-cpp
Ultra fast http server

## Running in docker
```
docker build --no-cache -t http-server .
docker run --rm -p 8080:8080 http-server:latest 
```

## Install http-server library

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build
```

## Quickstart

This is a C++ HTTP server library with examples. It uses CMake for building.

To build the project:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON -DBUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
```

To run the example server:
```bash
./build/examples/server/server
```

## Key Concepts

- The server uses a non-blocking I/O event loop with `poll()`
- Supports routing with parameterized paths (e.g., `/api/v1/users/:id`)
- Route handlers follow the signature: `void(const http::Request& req, http::Response& res)`
- Uses OpenSSL for SSL support
- Static file serving from `assets/` directory in examples

## Build Options

Build options are controlled by CMake variables:
- `BUILD_EXAMPLES` (default ON): Build example applications
- `BUILD_TESTS` (default OFF): Build tests
- `ENABLE_CLANG_TIDY` (default OFF): Enable clang-tidy static analysis

You can override these when running cmake.

## Environment

The server uses compile-time constants for configuration:
- `KEEPALIVE_MAX_COUNT` (default 96)
- `CONNECTION_TIMEOUT_SECOND` (default 3)
- `READ_BUFFER_SIZE` (default 4096)
- `CLIENT_TIMEOUT_SECONDS` (default 10)

## Framework Details

The server follows the standard C++ socket API and uses:
- Standard library (`<chrono>`, `<thread>`)
- POSIX sockets for connection handling
- OpenSSL for SSL/TLS support
