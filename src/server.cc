#include "server.h"

#include <unistd.h>

#include <chrono>
#include <thread>

#include "response.h"
#include "utils.h"
#include "websocket.h"

namespace http {

    int Server::start() {
        // open socket
        if ((sockfd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            log.error("Socket creation failed");
            exit(1);
        }

        log.debug("Server socket created sucessfully");

        // Set SO_REUSEADDR in socket options
        // For UDP SO_REUSEADDR may mean some problems...
        int optval = 1;
        if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) {
            log.error("Setting to allow multiple connection failed");
            exit(2);
        }

        log.debug("Server set to allow multiple connection sucessfully");

        // Link socket with address.
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(host_.c_str());
        server_addr.sin_port = htons(port_);
        if (bind(sockfd_, (struct sockaddr*)(&server_addr), sizeof(server_addr)) == -1) {
            log.error("Bind the server address failed");
            exit(3);
        }

        log.debug("Server binded to the host {} and port {} sucessfully", host_, port_);

        // Set listen socket to unblocking mode.
        setNonblockMode(sockfd_);

        // Server is ready to get SOMAXCONN connection requests (128).
        // This is *SHOULD* be enought to call accept and create child process.
        if (listen(sockfd_, SOMAXCONN) == -1) {
            log.error("Listening the server port failed");
            exit(4);
        }

        // initialize the running flag
        running_ = true;

        auto end_ = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_ - start_);
        log.info("Server started on http://{}:{} in {}", host_, port_, duration);

        handleRequests();

        return 0;
    }

    int Server::setNonblockMode(int fd) {
        int flags;
#if defined(O_NONBLOCK)
        if (-1 == (flags = fcntl(fd, F_GETFL, 0))) {
            flags = 0;
        }

        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
        flags = 1;
        return ioctl(fd, FIONBIO, &flags);
#endif
    }

    void Server::closeSocket(int fd, const std::string& label) {
        if (utils::isSocketAlive(fd)) {
            log.info("closing {} socket FD: {}", label, fd);
            if (close(fd) == -1) {
                log.error("Failed to close {} socket: {}", label, strerror(errno));
            }
        }
    }

    void Server::stop() {
        std::cout << "\n";

        // This is the cleanest "Algorithm" version
        std::ranges::for_each(wsRoutes, [this](const auto& ws) {
            closeSocket(ws.sockfd, "websocket");
        });

        closeSocket(sockfd_, "master");

        running_ = false;
        log.info("Server stopped successfully");
    }

    void Server::handleRequests() {
        while (running_) {
            std::vector<pollfd> descriptors;

            // Listen socket
            descriptors.push_back({});
            descriptors.back().fd = sockfd_;
            descriptors.back().events = POLLIN;

            // Client sockets using ranges
            std::ranges::transform(client_list_, std::back_inserter(descriptors), [](int fd) {
                pollfd pfd{};
                pfd.fd = fd;
                pfd.events = POLLIN;
                return pfd;
            });

            int rc = poll(descriptors.data(), static_cast<nfds_t>(descriptors.size()), POLL_TIMEOUT);
            if (rc < 0) {
                if (errno == EINTR) continue;
                log.error("poll failed: {}", strerror(errno));
                break;
            }

            std::vector<int> closedSockets;

            // Use ranges to process events instead of raw loop
            std::ranges::for_each(descriptors, [this, &closedSockets](pollfd& pfd) {
                if (pfd.revents == 0) return;

                if (pfd.fd == sockfd_) {
                    handleListenSocket(pfd);
                } else {
                    handleClientSocket(pfd, closedSockets);
                }
            });

            // Remove closed sockets from client_list_
            std::erase_if(client_list_, [&](int fd) {
                return std::ranges::contains(closedSockets, fd);
            });
        }
    }

    // Helper methods to keep logic clean
    void Server::handleListenSocket(const pollfd& pfd) {
        if (pfd.revents & POLLIN) {
            while (true) {
                sockaddr_in client_addr{};
                socklen_t slen = sizeof(client_addr);

                int client_fd = accept(sockfd_, reinterpret_cast<sockaddr*>(&client_addr), &slen);
                if (client_fd < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                    log.error("accept failed: {}", strerror(errno));
                    break;
                }

                setNonblockMode(client_fd);
                client_list_.insert(client_fd);
            }
        }
    }

    void Server::handleClientSocket(const pollfd& pfd, std::vector<int>& closedSockets) {
        int fd = pfd.fd;

        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            log.debug("Closing fd={} revents={}", fd, pfd.revents);
            shutdown(fd, SHUT_RDWR);
            close(fd);
            closedSockets.push_back(fd);
            return;
        }

        if (pfd.revents & POLLIN) {
            char buf[BUFFER_SIZE];
            ssize_t len = recv(fd, buf, sizeof(buf), MSG_NOSIGNAL);

            if (len > 0) {
                performRequest(fd, buf, len);
            } else if (len == 0) {
                log.debug("Peer closed fd={}", fd);
                shutdown(fd, SHUT_RDWR);
                close(fd);
                closedSockets.push_back(fd);
            } else {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    log.error("recv failed fd={} errno={} ({})", fd, errno, strerror(errno));
                    shutdown(fd, SHUT_RDWR);
                    close(fd);
                    closedSockets.push_back(fd);
                }
            }
        }
    }

    void Server::performRequest(const int sd, const char* buffer, const ssize_t nread) {
        std::string raw_request(buffer, nread);

        log.info("Request {}", raw_request);

        // Parse the request line to find method and path
        http::Request req(raw_request);
        Response res(default_headers_, sd);

        // Pre-routing handler
        if (pre_routing_handler_) {
            pre_routing_handler_(req, res);
        }

        // is websocket requests
        auto opcode = getWebSocketFrame(raw_request);
        if (opcode.has_value()) {
            log.debug("Websocket status: {}", toWsOpcodeString(opcode.value()));
            auto route = getWsRouteBySocketId(sd);
            if (route.has_value()) {
                if (opcode.value() == WsOpcode::Close) {
                    return;
                }

                WebSocket ws(sd, raw_request, route->query);
                route->handler(ws);
            }
            return;
        }

        // HTTP websocket handshake
        if (processWebsocketHandshake(sd, req)) {
            log.info("Norm path: {}", req.normalize_path());
            updateWsRoute(req.normalize_path(), req.query(), sd);
            return;
        }

        // handle HTTP request
        std::string body = handleRoute(req, res);
        // send server response
        sendResponse(sd, body);
    }

    bool Server::sendResponse(const int sd, std::string& body) {
        std::string_view data = body;

        // Recursive lambda definition
        std::function<bool()> sendAll = [&]() -> bool {
            ssize_t written = send(sd, data.data(), data.size(), MSG_NOSIGNAL);

            if (written == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    return sendAll();  // Recurse
                } else {
                    log.warning("Error writing response body: {}", strerror(errno));
                    return false;
                }
            }

            if (static_cast<size_t>(written) < data.size()) {
                data.remove_prefix(written);
                return sendAll();  // Recurse
            }

            return true;
        };

        return sendAll();
    }

    std::string Server::handleRoute(http::Request& req, http::Response& res) {
        res.setStaticDirectory(static_directory_);
        http::request_handler handler;
        if (router_.match(&req, &handler)) {
            handler(req, res);
        } else if (req.method() == "GET" || req.method() == "HEAD") {
            handleStaticResource(req, res);
        }

        // Post-routing handler
        if (post_routing_handler_) {
            post_routing_handler_(req, res);
        }

        return res.build();
    }

    void Server::handleStaticResource(http::Request& req, http::Response& res) {
        std::string path = static_directory_ + req.path();
        struct stat file_stat;

        if (stat(path.c_str(), &file_stat) == 0) {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (file) {
                // Get file size and read
                size_t size = file.tellg();
                file.seekg(0, std::ios::beg);

                std::string content(size, '\0');
                file.read(content.data(), size);
                file.close();

                // set basic headers
                res.setHeader("Content-Type", req.mimeType().value());
                res.setHeader("Cache-Control", "public, max-age=3600");
                res.setHeader("Content-Length", std::to_string(size));

                auto last_modified = ::utils::fileMtimeToHttpDate(file_stat.st_mtime);
                if (!last_modified.empty()) {
                    res.setHeader("Last-Modified", last_modified);
                }

                auto etag = ::utils::computeEtag(static_cast<size_t>(file_stat.st_mtime), file_stat.st_size);
                if (!etag.empty()) {
                    res.setHeader("ETag", etag);
                }

                // set content
                res << content;
            }
        } else {
            res.setContent<ContentType::PLAIN_TEXT, Status::not_found>("Not Found");
        }
    }

    bool Server::processWebsocketHandshake(const int sd, http::Request& req) {
        const auto& headers = req.headers();

        // Fast path: check both headers in one go with minimal string operations
        auto upgrade_it = headers.find("Upgrade");
        if (upgrade_it == headers.end()) {
            return false;
        }

        auto connection_it = headers.find("Connection");
        if (connection_it == headers.end() && connection_it->second != "Upgrade") {
            return false;
        }

        // Direct comparison without creating temporary strings
        const std::string_view& upgrade_value = upgrade_it->second;
        if (upgrade_value.size() != 9 || (upgrade_value[0] != 'w' && upgrade_value[0] != 'W') ||
            upgrade_value.substr(0, 9) != "websocket") {
            return false;
        }

        // Check Sec-WebSocket-Key
        auto key_it = headers.find("Sec-WebSocket-Key");
        if (key_it == headers.end()) {
            return false;
        }

        const std::string_view& client_key = key_it->second;
        if (client_key.empty()) {
            return false;
        }

        // Check Sec-WebSocket-Key is a valid base64-encoded 16-byte value (24 chars)
        // RFC 6455 Section 4.2.1
        if (client_key.size() != 24 || client_key[22] != '=' || client_key[23] != '=') {
            return false;
        }

        log.debug("Websocket request for accepting with client key {}",
                  std::string(client_key));  // Only convert to string for logging

        // Base64 encode the hash
        const std::string_view magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        std::string accept_key = ::utils::base64_encode(::utils::sha1(std::string(client_key) + std::string(magic)));

        std::string response;
        response =
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: ";

        response += accept_key;
        response += "\r\n\r\n";

        log.debug("Websocket accepted with key {}", accept_key);

        return sendResponse(sd, response);
    }

}  // namespace http
