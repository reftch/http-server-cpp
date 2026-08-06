# Stage 1: Build stage
FROM alpine:3.20 AS build

RUN apk add --no-cache \
    build-base \
    cmake \
    git \
    curl \
    bash \
    musl-dev \
    openssl \
    openssl-dev \
    openssl-libs-static \
    zlib-dev \
    linux-headers \
    ca-certificates

WORKDIR /app

# Copy the source code into the container
COPY examples ./examples
COPY src ./src
COPY test ./test
COPY CMakeLists.txt ./CMakeLists.txt

# Compile in release mode
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXE_LINKER_FLAGS="-static"

RUN cmake --build build -j$(nproc) --target rest

# Create cerificates
#RUN openssl req -x509 -noenc -keyout key.pem -out cert.pem -subj /CN=0.0.0.0

# Production
FROM scratch

#COPY --from=build /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/
#COPY --from=build /app/cert.pem /cert.pem
#COPY --from=build /app/key.pem /key.pem

COPY --from=build /app/build/examples/rest/rest /application

CMD [ "/application" ]
