#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

#ifdef HTTP_OPENSSL_SUPPORT
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "logger.h"
#include "utils.h"

namespace http {

    enum class Result : int { Fail = 0, Text = 1, Binary = 2 };

    // For WebSocket protocols
    enum class WsProtocol {
        WS,
        WSS,  // WebSocket Secure
    };

    enum class WsOpcode : uint8_t {
        Continuation = 0x0,
        Text = 0x1,
        Binary = 0x2,
        Close = 0x8,
        Ping = 0x9,
        Pong = 0xA,
    };

    bool isWebSocketFrame(const std::string& data);

    class WebSocket {
       private:
        // logger
        Logger& log = Logger::getInstance();

        struct Frame {
            bool fin;
            WsOpcode opcode;
            bool mask;
            std::size_t payload_length;
            std::array<uint8_t, 4> masking_key;
            std::vector<uint8_t> payload_data;
            std::string text_payload;  // For text frames only
            int sockfd;

#ifdef HTTP_OPENSSL_SUPPORT
            SSL* ssl = nullptr;
#endif

            Frame() : fin(false), opcode(WsOpcode::Continuation), mask(false), payload_length(0) {}
            Frame(int sfd) : fin(false), opcode(WsOpcode::Continuation), mask(false), payload_length(0), sockfd(sfd) {}

#ifdef HTTP_OPENSSL_SUPPORT
            Frame(int32_t sfd, SSL* ssl)
                : fin(false), opcode(WsOpcode::Continuation), mask(false), payload_length(0), sockfd(sfd), ssl(ssl) {}
#endif
        };

       public:
        WebSocket(int sockfd, const std::string& raw_request)
            : frame{sockfd}, byte_data(raw_request.begin(), raw_request.end()), isOpen(true) {
            log.debug("Websocket open FD={}", sockfd);
        }
#ifdef HTTP_OPENSSL_SUPPORT
        WebSocket(int sockfd, SSL* ssl, const std::string& raw_request)
            : frame{sockfd, ssl}, byte_data(raw_request.begin(), raw_request.end()), isOpen(true) {}
#endif

        void close() {
            isOpen = false;
            log.info("Close socket FD={}", frame.sockfd);
            // ::close(frame.sockfd);
#ifdef HTTP_OPENSSL_SUPPORT
            if (frame.ssl) {
                SSL_free(frame.ssl);
                frame.ssl = nullptr;
            }

            EVP_cleanup();
#endif
        }

        ssize_t send(const std::string& msg) {
            int sockfd = frame.sockfd;
            log.debug("Is socket FD={} is alive: {}", sockfd, utils::isSocketAlive(sockfd));

            if (!utils::isSocketAlive(sockfd)) {
                log.debug("Websocket is closed");
                return -1;
            }

            // Creates WebSocket frame from string
#ifndef HTTP_OPENSSL_SUPPORT
            auto response = writeFrame(msg, frame.fin, frame.opcode);
            if (response.empty()) {
                return -1;  // Return error if frame creation failed
            }
            // Send the frame to the socket
            ssize_t sent = ::send(sockfd, response.data(), response.size(), 0);
#else
            // auto response = writeFrame(msg, true, WsOpcode::Text);
            auto response = writeFrame(msg, frame.fin, WsOpcode::Text);
            if (response.empty()) {
                return -1;  // Return error if frame creation failed
            }
            // Send the frame to the socket
            ssize_t sent = SSL_write(frame.ssl, response.data(), response.size());
            // If we get a positive value, check if connection is actually alive
            // Verify connection is still alive by checking if we can read
            char test_buf[1];
            int result = SSL_peek(frame.ssl, test_buf, 1);
            if (result == 0) {
                std::cout << "Connection closed during peek\n";
                return -1;
            }

#endif
            if (sent < 0) {
                perror("WebSocket send failed");
            }
            return sent;
        }

        [[nodiscard]]
        Result read(std::string& msg) {
            if (!readFrame(byte_data)) {
                return Result::Fail;
            }

            if (frame.opcode == WsOpcode::Close) {
                close();
                return Result::Fail;
            }

            msg = frame.text_payload;
            return frame.opcode == WsOpcode::Text ? Result::Text : Result::Binary;
        }

        friend Result operator>>(WebSocket& ws, std::string& msg) { return ws.read(msg); }
        // For sending messages - returns ssize_t like send() does
        friend ssize_t operator<<(WebSocket& ws, const std::string& msg) { return ws.send(msg); }

       private:
        Frame frame;
        std::vector<uint8_t> byte_data;
        bool isOpen = false;

        // Read a single WebSocket frame from raw bytes
        bool readFrame(const std::vector<uint8_t>& data);

        // Create a WebSocket frame for sending
        std::vector<uint8_t> writeFrame(const std::string& message, bool fin, WsOpcode opcode, bool mask = false);
    };

    constexpr std::string_view toWsString(WsProtocol protocol) {
        switch (protocol) {
            case WsProtocol::WS:
                return "ws";
            case WsProtocol::WSS:
                return "wss";
        }
        return "ws";  // or handle as unreachable
    }

}  // namespace http
#endif