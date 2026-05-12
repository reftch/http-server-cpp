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
