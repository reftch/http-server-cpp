#pragma once
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "utils.h"

class WebSocketClient {
   private:
    int socket_fd_;
    std::string host_;
    int port_;
    bool connected_;

   public:
    explicit WebSocketClient(const std::string& host, int port) : host_(host), port_(port), connected_(false) {
        socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd_ == -1) {
            throw std::runtime_error("Failed to create socket");
        }
    }

    ~WebSocketClient() { disconnect(); }

    bool connect() {
        struct sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);

        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
            return false;
        }

        if (::connect(socket_fd_, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            return false;
        }

        connected_ = true;
        return true;
    }

    void disconnect() {
        if (connected_) {
            ::close(socket_fd_);
            connected_ = false;
        }
    }

    bool handshake(const std::string& path = "/") {
        if (!connected_) return false;

        std::string key = generateWebsocketKey();

        // Create WebSocket upgrade request
        std::ostringstream request;
        request << "GET " << path << " HTTP/1.1\r\n";
        request << "Host: " << host_ << ":" << port_ << "\r\n";
        request << "Upgrade: websocket\r\n";
        request << "Connection: Upgrade\r\n";
        request << "Sec-WebSocket-Key: " << key << "\r\n";
        request << "Sec-WebSocket-Version: 13\r\n";
        request << "\r\n";

        std::string req_str = request.str();

        if (::send(socket_fd_, req_str.c_str(), req_str.length(), 0) < 0) {
            return false;
        }

        // Read response
        std::array<char, 4096> buffer{};
        ssize_t bytes_received = ::recv(socket_fd_, buffer.data(), buffer.size() - 1, 0);

        if (bytes_received <= 0) return false;

        std::string response(buffer.begin(), buffer.begin() + bytes_received);

        // Check for successful upgrade
        return response.find("101 Switching Protocols") != std::string::npos;
    }

    bool sendMessage(const std::string& message, bool is_binary = false) {
        if (!connected_) return false;

        // Frame the message according to WebSocket protocol (simplified)
        std::vector<uint8_t> frame;

        // FIN bit + opcode (text frame = 0x1)
        uint8_t first_byte = 0x80 | (is_binary ? 0x02 : 0x01);
        frame.push_back(first_byte);

        // Payload length
        size_t len = message.length();
        if (len <= 125) {
            frame.push_back(static_cast<uint8_t>(len));
        } else if (len <= 65535) {
            frame.push_back(126);
            frame.push_back((len >> 8) & 0xFF);
            frame.push_back(len & 0xFF);
        } else {
            frame.push_back(127);
            for (int i = 7; i >= 0; --i) {
                frame.push_back((len >> (8 * i)) & 0xFF);
            }
        }

        // Add message data
        frame.insert(frame.end(), message.begin(), message.end());

        if (::send(socket_fd_, frame.data(), frame.size(), 0) < 0) {
            return false;
        }

        return true;
    }

    // Add this method for sending ping frames
    bool sendPing(const std::string& payload = "") {
        if (!connected_) return false;

        std::vector<uint8_t> frame;

        // Ping frame: FIN bit set + opcode 0x9
        frame.push_back(0x89);  // FIN = 1, RSV = 0, opcode = 0x9 (ping)

        // Payload length
        size_t len = payload.length();
        if (len <= 125) {
            frame.push_back(static_cast<uint8_t>(len));
        } else if (len <= 65535) {
            frame.push_back(126);
            frame.push_back((len >> 8) & 0xFF);
            frame.push_back(len & 0xFF);
        } else {
            frame.push_back(127);
            for (int i = 7; i >= 0; --i) {
                frame.push_back((len >> (8 * i)) & 0xFF);
            }
        }

        // Add payload data
        frame.insert(frame.end(), payload.begin(), payload.end());

        if (::send(socket_fd_, frame.data(), frame.size(), 0) < 0) {
            return false;
        }

        return true;
    }

    std::string receiveMessage() {
        if (!connected_) return {};

        std::array<char, 4096> buffer{};
        ssize_t bytes_received = ::recv(socket_fd_, buffer.data(), buffer.size() - 1, 0);
        if (bytes_received <= 0) return {};

        return std::string(buffer.begin(), buffer.begin() + bytes_received);
    }

   private:
    // Generate WebSocket key
    std::string generateWebsocketKey() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);

        std::array<char, 16> key;
        for (auto& c : key) {
            c = static_cast<char>(dis(gen));
        }

        return utils::base64_encode(std::string(key.begin(), key.end()));
    }
};