# Stage 1: Build stage
FROM ubuntu:latest AS build

# Install build-essential for compiling C++ code
RUN apt-get update && apt-get install -y build-essential cmake

# Set the working directory
WORKDIR /app

# Copy the source code into the container
COPY . .

# Compile in release mode  
RUN cmake -DCMAKE_BUILD_TYPE=Release .
RUN make all

# Stage 2: Runtime stage
FROM scratch

# Copy the static binary from the build stage
COPY --from=build /app/server /server

# Command to run the binary
CMD ["/server"]