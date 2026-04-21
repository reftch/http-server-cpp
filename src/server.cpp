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
/**
 * Constructor implementation.
 */
server::server(const std::string& host, const std::string& port) : host_(host), port_(port) {
    // std::cout << "[Server Setup] Initializing server for host: " << host_
    //           << ", port: " << port_ << std::endl;
}

static const char* okResp =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain; charset=utf-8\r\n"
    "Content-Length: 19\r\n"
    "Connection: keep-alive\r\n\r\n"
    "hello from pure c++";

/**
 * Implementation of the start method.
 * In a real application, this method would set up sockets and begin listening.
 */
int server::start() {
    // std::cout << "Server started on http:// " << host_ << ":" << port_ << std::endl;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    int one = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(std::stoi(this->port_));

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0 || listen(sockfd, 100) < 0) {
        perror("bind/listen");
        close(sockfd);
        return 1;
    }

    if (setNonblocking(sockfd) < 0) {
        perror("setNonblocking");
        close(sockfd);
        return 1;
    }

    std::vector<Ctx> ctxs(MAX_CONNS);
    for (int i = 0; i < MAX_CONNS; ++i) {
        ctxs[i].fd = -1;
    }

    // fd -> index (simple for FDs < MAX_CONNS)
    std::vector<int> fd_to_idx(MAX_CONNS, -1);
    fd_to_idx[sockfd] = sockfd;  // Use FD as index

    std::cout << "server listening on http://" << this->host_ << ":" << this->port_ << std::endl;

    while (true) {
        // Build pollfd array from active FDs
        std::vector<struct pollfd> pfds;
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
                struct sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
                if (connfd < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                    perror("accept");
                    continue;
                }

                if (connfd >= MAX_CONNS) {
                    std::cout << "too many connections\n";
                    close(connfd);
                    continue;
                }

                if (setNonblocking(connfd) < 0) {
                    close(connfd);
                    continue;
                }

                fd_to_idx[connfd] = connfd;
                ctxs[connfd].fd = connfd;

                // std::cout << "accepted connfd " << connfd << "\n";
                continue;
            }

            // Existing client
            int idx = fd_to_idx[fd];
            if (idx < 0 || idx >= MAX_CONNS || ctxs[idx].fd != fd) continue;

            Ctx& ctx = ctxs[idx];

            if (pfds[j].revents & POLLHUP || pfds[j].revents & POLLERR) {
                close(fd);
                fd_to_idx[fd] = -1;
                ctx.fd = -1;
                continue;
            }

            if (pfds[j].revents & POLLIN) {
                char dummy[4096];
                ssize_t nread = read(fd, dummy, sizeof(dummy));
                if (nread > 0) {
                    // std::cout << "read " << nread << " bytes on fd " << fd << "\n";
                    write(fd, okResp, strlen(okResp));
                } else if (nread == 0) {
                    close(fd);
                    fd_to_idx[fd] = -1;
                    ctx.fd = -1;
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // More data coming
                    } else {
                        std::cout << "read error on fd " << fd << "\n";
                        close(fd);
                        fd_to_idx[fd] = -1;
                        ctx.fd = -1;
                    }
                }
            }
        }
    }

    close(sockfd);
    return 0;
}

int server::setNonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) return -1;
    return 0;
}

}  // namespace http::server