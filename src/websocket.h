#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstring>
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

            Frame() : fin(false), opcode(WsOpcode::Continuation), mask(false), payload_length(0) {}
            Frame(uint32_t sfd)
                : fin(false), opcode(WsOpcode::Continuation), mask(false), payload_length(0), sockfd(sfd) {}
        };

       public:
        explicit WebSocket(uint32_t sockfd, const std::string& raw_request)
            : frame{sockfd}, byte_data(raw_request.begin(), raw_request.end()) {}

        [[nodiscard]]
        ssize_t send(const std::string& msg);
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
        std::vector<uint8_t> writeFrame(const std::string& message, bool fin = true, WsOpcode opcode = WsOpcode::Text);

        // Create a masked WebSocket frame (for client-to-server messages)
        std::vector<uint8_t> writeMaskedFrame(const std::string& message, bool fin = true, uint8_t opcode = 1);

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