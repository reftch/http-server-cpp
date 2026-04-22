// server.cpp
#include "server.hpp"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

namespace http::server {

static const char* okResp =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain; charset=utf-8\r\n"
    "Content-Length: 19\r\n"
    "Connection: keep-alive\r\n\r\n"
    "hello from pure c++";

/**
 * Constructor implementation
 * Initializes the server with the specified host and port configuration.
 * Does not establish any network connections or start listening.
 *
 * @param host The hostname or IP address to bind to
 * @param port The port number to listen on
 */
server::server(const std::string& host, const std::string& port) : host_(host), port_(port) {
    start_time = std::chrono::high_resolution_clock::now();

    // Initialize connection tracking structures
    ctxs.reserve(MAX_CONNS);
    for (int i = 0; i < MAX_CONNS; ++i) {
        ctxs[i].fd = -1;
    }

    // fd -> index mapping (simple for FDs < MAX_CONNS)
    fd_to_idx.reserve(MAX_CONNS);
    fd_to_idx.resize(MAX_CONNS, -1);
}

/**
 * Implementation of the start method.
 * Sets up the server socket, binds to the specified address, and enters the main event loop.
 * The server will continuously listen for new connections and handle existing connections.
 *
 * @return 0 on successful start, non-zero on error
 */
int server::start() {
    // create server socket
    // AF_INET: IPv4 protocol, SOCK_STREAM: TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    // set sockek options
    // SO_REUSEADDR: allow local address reuse
    int one = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    // define server address
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;              // INADDR_ANY: Accept connections on any IP
    addr.sin_port = htons(std::stoi(this->port_));  // htons(): Converts port to network byte order

    // bind Socket to Address and listen for incoming Connections
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0 || listen(sockfd, 100) < 0) {
        perror("bind/listen");
        close(sockfd);
        return 1;
    }

    // fd -> index mapping (simple for FDs < MAX_CONNS)
    fd_to_idx[sockfd] = sockfd;  // Use FD as index

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    std::cout << "server listening on http://" << this->host_ << ":" << this->port_ << " in " << duration << std::endl;

    while (true) {
        // Build pollfd array from active FDs
        pfds.clear();
        pfds.reserve(MAX_CONNS);
        int max_fd = sockfd;

        // Add listening socket
        struct pollfd pfd_sock = {sockfd, POLLIN, 0};
        pfds.push_back(pfd_sock);

        // Add client sockets
        for (int i = 0; i < MAX_CONNS; ++i) {
            if (ctxs[i].fd != -1) {
                struct pollfd pfd = {ctxs[i].fd, POLLIN, 0};
                pfds.push_back(pfd);
                if (ctxs[i].fd > max_fd) max_fd = ctxs[i].fd;
            }
        }

        // Wait for events (infinite timeout)
        int n = poll(pfds.data(), pfds.size(), -1);
        if (n < 0) {
            perror("poll");
            break;
        }
        if (n == 0) continue;

        // Process events
        for (size_t j = 0; j < pfds.size(); ++j) {
            if (pfds[j].revents == 0) continue;

            int fd = pfds[j].fd;

            if (fd == sockfd) {
                // New connection
                acceptConnection(sockfd);
            } else {
                // Existing client
                handleRequest(fd, j);
            }
        }
    }

    // close socket
    close(sockfd);
    return 0;
}

/**
 * Handle new incoming connection.
 * Accepts a new connection from the listening socket and sets it up for non-blocking I/O.
 * Adds the new connection to the connection tracking system.
 *
 * @param sockfd Listening socket file descriptor
 * @return 0 on success, -1 on error
 */
int server::acceptConnection(int sockfd) {
    // accept client connection
    struct sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
    if (connfd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return -1;
        perror("accept");
        return -1;
    }

    if (connfd >= MAX_CONNS) {
        std::cout << "too many connections" << std::endl;
        close(connfd);
        return -1;
    }

    // set socket to non-blocking mode
    if (setNonblocking(connfd) < 0) {
        close(connfd);
        return -1;
    }

    fd_to_idx[connfd] = connfd;
    ctxs[connfd].fd = connfd;

    return 0;
}

/**
 * Handle an incoming HTTP request on a given connection.
 * Processes data received on the connection and handles the request logic.
 *
 * @param fd File descriptor of the connection
 * @param i Index in the internal context vector
 * @return 0 on success, -1 on error or connection close
 */
int server::handleRequest(int fd, int i) {
    int idx = fd_to_idx[fd];
    if (idx < 0 || idx >= MAX_CONNS || ctxs[idx].fd != fd) return -1;

    Ctx& ctx = ctxs[idx];

    if (pfds[i].revents & POLLHUP || pfds[i].revents & POLLERR) {
        close(fd);
        fd_to_idx[fd] = -1;
        ctx.fd = -1;
        return -1;
    }

    if (pfds[i].revents & POLLIN) {
        char dummy[4096];
        ssize_t nread = read(fd, dummy, sizeof(dummy));
        if (nread > 0) {
            // Process incoming data and send response
            write(fd, okResp, strlen(okResp));
        } else if (nread == 0) {
            // Client closed connection
            close(fd);
            fd_to_idx[fd] = -1;
            ctx.fd = -1;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // More data coming
            } else {
                // Handle read error
                std::cout << "read error on fd " << fd << "\n";
                close(fd);
                fd_to_idx[fd] = -1;
                ctx.fd = -1;
            }
        }
    }

    return 0;
}

/**
 * Set socket to non-blocking mode.
 * Configures the socket to operate in non-blocking mode, which is essential for efficient
 * polling-based I/O multiplexing.
 *
 * @param fd Socket file descriptor
 * @return 0 on success, -1 on error
 */
int server::setNonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) return -1;
    return 0;
}

}  // namespace http::server