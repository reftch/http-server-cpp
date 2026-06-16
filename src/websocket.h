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
        struct Frame {
            bool fin;
            WsOpcode opcode;
            bool mask;
            std::size_t payload_length;
            std::array<uint8_t, 4> masking_key;
            std::vector<uint8_t> payload_data;
            std::string text_payload;  // For text frames only
            uint32_t sockfd;

#ifdef HTTP_OPENSSL_SUPPORT
            SSL* ssl = nullptr;
#endif

            Frame() : fin(false), opcode(WsOpcode::Continuation), mask(false), payload_length(0) {}
            Frame(uint32_t sfd)
                : fin(false), opcode(WsOpcode::Continuation), mask(false), payload_length(0), sockfd(sfd) {}

#ifdef HTTP_OPENSSL_SUPPORT
            Frame(uint32_t sfd, SSL* ssl)
                : fin(false), opcode(WsOpcode::Continuation), mask(false), payload_length(0), sockfd(sfd), ssl(ssl) {}
#endif
        };

       public:
        WebSocket(uint32_t sockfd, const std::string& raw_request)
            : frame{sockfd}, byte_data(raw_request.begin(), raw_request.end()) {}
#ifdef HTTP_OPENSSL_SUPPORT
        WebSocket(uint32_t sockfd, SSL* ssl, const std::string& raw_request)
            : frame{sockfd, ssl}, byte_data(raw_request.begin(), raw_request.end()) {}
#endif

        ssize_t send(const std::string& msg) {
            if (!isSocketAlive(frame.sockfd)) {
                return -1;
            }

            // Creates WebSocket frame from string
#ifndef HTTP_OPENSSL_SUPPORT
            auto response = writeFrame(msg, frame.fin, frame.opcode);
            if (response.empty()) {
                return -1;  // Return error if frame creation failed
            }
            // Send the frame to the socket
            ssize_t sent = ::send(frame.sockfd, response.data(), response.size(), 0);
#else
            // auto response = writeFrame(msg, true, WsOpcode::Text);
            auto response = writeFrame(msg, true, WsOpcode::Text);
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
            std::cout << "Result: " << sent << '\n';
            if (sent < 0) {
                perror("WebSocket send failed");
            }
            return sent;
        }

        [[nodiscard]]
        Result read(std::string& msg);

        friend Result operator>>(WebSocket& ws, std::string& msg) { return ws.read(msg); }
        // For sending messages - returns ssize_t like send() does
        friend ssize_t operator<<(WebSocket& ws, const std::string& msg) { return ws.send(msg); }

       private:
        Frame frame;
        std::vector<uint8_t> byte_data;

        // Read a single WebSocket frame from raw bytes
        bool readFrame(const std::vector<uint8_t>& data);

        // Create a WebSocket frame for sending
        std::vector<uint8_t> writeFrame(const std::string& message, bool fin, WsOpcode opcode, bool mask = false);

        bool isSocketAlive(int sockfd);
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