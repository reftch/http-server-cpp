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
    namespace ws {

        enum class Result : int { Fail = 0, Text = 1, Binary = 2 };

        // For WebSocket protocols
        enum class Protocol {
            WS,
            WSS,  // WebSocket Secure
        };

        enum class Opcode : uint8_t {
            Continuation = 0x0,
            Text = 0x1,
            Binary = 0x2,
            Close = 0x8,
            Ping = 0x9,
            Pong = 0xA,
        };

        bool isWebSocketFrame(const std::string& data);

        class Response {
           private:
            struct Frame {
                bool fin;
                Opcode opcode;
                bool mask;
                std::size_t payload_length;
                std::array<uint8_t, 4> masking_key;
                std::vector<uint8_t> payload_data;
                std::string text_payload;  // For text frames only
                uint32_t sockfd;

                Frame() : fin(false), opcode(Opcode::Continuation), mask(false), payload_length(0) {}
                Frame(uint32_t sfd)
                    : fin(false), opcode(Opcode::Continuation), mask(false), payload_length(0), sockfd(sfd) {}
            };

           public:
            explicit Response(uint32_t sockfd, const std::string& raw_request)
                : frame{sockfd}, byte_data(raw_request.begin(), raw_request.end()) {}

            [[nodiscard]]
            ssize_t send(const std::string& msg);
            [[nodiscard]]
            Result read(std::string& msg);

           private:
            Frame frame;
            std::vector<uint8_t> byte_data;

            // Read a single WebSocket frame from raw bytes
            bool readFrame(const std::vector<uint8_t>& data);

            // Create a WebSocket frame for sending
            std::vector<uint8_t> writeFrame(const std::string& message, bool fin = true, Opcode opcode = Opcode::Text);

            // Create a masked WebSocket frame (for client-to-server messages)
            std::vector<uint8_t> writeMaskedFrame(const std::string& message, bool fin = true, uint8_t opcode = 1);
        };

    }  // namespace ws

}  // namespace http
#endif