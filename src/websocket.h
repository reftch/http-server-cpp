#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

#ifdef HTTP_OPENSSL_SUPPORT
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "logger.h"

namespace http {

    enum class Result : int { Fail = 0, Text = 1, Binary = 2 };

    enum class WsOpcode : uint8_t {
        Continuation = 0x0,
        Text = 0x1,
        Binary = 0x2,
        Close = 0x8,
        Ping = 0x9,
        Pong = 0xA,
    };

    struct Frame {
        bool fin;
        WsOpcode opcode;
        bool mask;
        std::size_t payload_length;
        std::array<uint8_t, 4> masking_key;
        std::vector<uint8_t> payload_data;
        std::string text_payload;  // For text frames only
        int sockfd;

        Frame() : fin(false), opcode(WsOpcode::Continuation), mask(false), payload_length(0) {}
        Frame(int sfd) : fin(false), opcode(WsOpcode::Continuation), mask(false), payload_length(0), sockfd(sfd) {}

#ifdef HTTP_OPENSSL_SUPPORT
        SSL* ssl = nullptr;

        Frame(int32_t sfd, SSL* ssl)
            : fin(false), opcode(WsOpcode::Continuation), mask(false), payload_length(0), sockfd(sfd), ssl(ssl) {}
#endif
    };

    class WebSocket {
       private:
        // logger
        Logger& log = Logger::getInstance();

       public:
        WebSocket(int sockfd, const std::string& raw_request, const std::unordered_map<std::string, std::string> query)
            : frame{sockfd}, byte_data(raw_request.begin(), raw_request.end()), query_(query), isOpen(true) {
            log.debug("Websocket open FD={}", sockfd);
        }
#ifdef HTTP_OPENSSL_SUPPORT
        WebSocket(int sockfd, SSL* ssl, const std::string& raw_request,
                  const std::unordered_map<std::string, std::string> query)
            : frame{sockfd, ssl}, byte_data(raw_request.begin(), raw_request.end()), query_(query), isOpen(true) {}
#endif

        Frame getFrame() { return frame; }
        const std::unordered_map<std::string, std::string>& query() const { return query_; }

        void close() {
            isOpen = false;
            log.debug("Websocket {} will closed", frame.sockfd);
            ::close(frame.sockfd);
#ifdef HTTP_OPENSSL_SUPPORT
            if (frame.ssl) {
                SSL_free(frame.ssl);
                frame.ssl = nullptr;
            }

            EVP_cleanup();
#endif
        }

        ssize_t send(const std::string& msg) {
            // int sockfd = frame.sockfd;
            //  log.debug("Is socket FD={} is alive: {}", sockfd, utils::isSocketAlive(sockfd));

            if (frame.opcode == WsOpcode::Close) {
                close();
                return -1;
            }

            // Creates WebSocket frame from string
#ifndef HTTP_OPENSSL_SUPPORT
            auto response = writeFrame(msg, frame.fin, frame.opcode);
            if (response.empty()) {
                return -1;  // Return error if frame creation failed
            }

            if (frame.opcode == WsOpcode::Close) {
                close();
                return -1;
            }

            // Send the frame to the socket
            ssize_t sent = ::send(frame.sockfd, response.data(), response.size(), 0);
#else
            // auto response = writeFrame(msg, true, WsOpcode::Text);
            auto response = writeFrame(msg, frame.fin, frame.opcode);
            // auto response = writeFrame(msg, frame.fin, WsOpcode::Text);
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
        std::unordered_map<std::string, std::string> query_;
        bool isOpen = false;

        // Read a single WebSocket frame from raw bytes
        bool readFrame(const std::vector<uint8_t>& data);

        // Create a WebSocket frame for sending
        std::vector<uint8_t> writeFrame(const std::string& message, bool fin, WsOpcode opcode, bool mask = false);
    };

    std::optional<WsOpcode> getWebSocketFrame(const std::string& data);

    constexpr std::string_view toWsOpcodeString(WsOpcode opcode) {
        switch (opcode) {
            case WsOpcode::Continuation:
                return "Continuation";
            case WsOpcode::Text:
                return "Text";
            case WsOpcode::Binary:
                return "Binary";
            case WsOpcode::Close:
                return "Close";
            case WsOpcode::Ping:
                return "Ping";
            case WsOpcode::Pong:
                return "Pong";
        }
        return "Unknown";  // This should never be reached for valid enum values
    }

}  // namespace http
#endif