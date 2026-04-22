# Stage 1: Build stage
FROM ubuntu:latest AS build

# Install build-essential for compiling C++ code
RUN apt-get update && apt-get install -y build-essential

# Set the working directory
WORKDIR /app

# Copy the source code into the container
COPY server.cpp .

# Compile the C++ code statically to ensure it doesn't depend on runtime libraries
RUN g++ -o server server.cpp -static -std=c++17 -O3

# Stage 2: Runtime stage
FROM scratch

# Copy the static binary from the build stage
COPY --from=build /app/server /server

# Command to run the binary
CMD ["/server"]