#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <string>

namespace http {
    namespace ws {

        struct Frame {
            bool fin;
            uint8_t opcode;
            bool mask;
            std::size_t payload_length;
            std::array<uint8_t, 4> masking_key;
            std::vector<uint8_t> payload_data;
            std::string text_payload;  // For text frames only
            std::string client_id;     // Client identifier
            std::string path;          // Request path

            Frame() : fin(false), opcode(0), mask(false), payload_length(0) {}
        };

        // For WebSocket protocols
        enum class Protocol {
            WS,
            WSS,  // WebSocket Secure
        };

        class Response {
           public:
            Response() = default;

            // Parse a single WebSocket frame from raw bytes
            Frame parseFrame(const std::vector<uint8_t>& data) {
                Frame frame;
                std::size_t offset = 0;

                if (data.size() < 2) {
                    return frame;  // Invalid frame
                }

                // Read FIN and RSV bits (1 byte)
                uint8_t first_byte = data[offset++];
                frame.fin = (first_byte & 0x80) != 0;
                frame.opcode = first_byte & 0x0F;

                // Read mask and payload length (1 byte)
                uint8_t second_byte = data[offset++];
                frame.mask = (second_byte & 0x80) != 0;
                frame.payload_length = second_byte & 0x7F;

                // Handle extended payload length
                if (frame.payload_length == 126) {
                    // 2-byte extended length
                    if (data.size() < offset + 2) return frame;  // Incomplete frame
                    frame.payload_length = (data[offset] << 8) | data[offset + 1];
                    offset += 2;
                } else if (frame.payload_length == 127) {
                    // 8-byte extended length
                    if (data.size() < offset + 8) return frame;  // Incomplete frame
                    frame.payload_length = 0;
                    for (int i = 0; i < 8; ++i) {
                        frame.payload_length |= (static_cast<std::size_t>(data[offset + i]) << (56 - 8 * i));
                    }
                    offset += 8;
                }

                // Read masking key if present
                if (frame.mask) {
                    if (data.size() < offset + 4) return frame;  // Incomplete frame
                    for (int i = 0; i < 4; ++i) {
                        frame.masking_key[i] = data[offset + i];
                    }
                    offset += 4;
                }

                // Read payload data
                if (data.size() < offset + frame.payload_length) return frame;  // Incomplete frame

                frame.payload_data.resize(frame.payload_length);
                for (std::size_t i = 0; i < frame.payload_length; ++i) {
                    frame.payload_data[i] = data[offset + i];
                    // Unmask if needed
                    if (frame.mask) {
                        frame.payload_data[i] ^= frame.masking_key[i % 4];
                    }
                }

                // Convert to text if it's a text frame
                if (frame.opcode == 1) {  // Text frame
                    frame.text_payload = std::string(frame.payload_data.begin(), frame.payload_data.end());
                }

                return frame;
            }

            // Parse WebSocket message (may consist of multiple frames)
            std::string parseMessage(const std::vector<uint8_t>& data) {
                // This is a simplified version - in practice, you'd need to handle
                // fragmented messages and reassembly. For now, we assume single frame.

                auto frame = parseFrame(data);
                return frame.text_payload;
            }

            // Helper function to check if data looks like a WebSocket frame
            bool isWebSocketFrame(const std::vector<uint8_t>& data) {
                if (data.size() < 2) return false;

                uint8_t first_byte = data[0];
                // uint8_t second_byte = data[1];

                // Check FIN bit and valid opcode
                bool fin = (first_byte & 0x80) != 0;
                uint8_t opcode = first_byte & 0x0F;

                return fin && (opcode <= 0xA);
            }

            // // Parse WebSocket frame from string (converts to vector<uint8_t>)
            // Frame parseFrameFromString(const std::string& data) {
            //     std::vector<uint8_t> byte_data(data.begin(), data.end());
            //     return parseFrame(byte_data);
            // }

            // Create a WebSocket frame for sending
            static std::vector<uint8_t> createFrame(const std::string& message, bool fin = true, uint8_t opcode = 1) {
                std::vector<uint8_t> frame;

                // First byte: FIN bit + RSV bits + OPCODE
                uint8_t first_byte = (fin ? 0x80 : 0x00) | (opcode & 0x0F);
                frame.push_back(first_byte);

                // Second byte: MASK bit + PAYLOAD LENGTH
                std::size_t payload_length = message.length();
                if (payload_length < 126) {
                    frame.push_back(static_cast<uint8_t>(payload_length));
                } else if (payload_length <= 0xFFFF) {
                    frame.push_back(126);  // 2-byte extended length
                    frame.push_back(static_cast<uint8_t>((payload_length >> 8) & 0xFF));
                    frame.push_back(static_cast<uint8_t>(payload_length & 0xFF));
                } else {
                    frame.push_back(127);  // 8-byte extended length
                    for (int i = 7; i >= 0; --i) {
                        frame.push_back(static_cast<uint8_t>((payload_length >> (8 * i)) & 0xFF));
                    }
                }

                // Add payload data
                for (char c : message) {
                    frame.push_back(static_cast<uint8_t>(c));
                }

                return frame;
            }

            // Create a masked WebSocket frame (for client-to-server messages)
            static std::vector<uint8_t> createMaskedFrame(const std::string& message, bool fin = true,
                                                          uint8_t opcode = 1) {
                std::vector<uint8_t> frame;

                // First byte: FIN bit + RSV bits + OPCODE
                uint8_t first_byte = (fin ? 0x80 : 0x00) | (opcode & 0x0F);
                frame.push_back(first_byte);

                // Second byte: MASK bit + PAYLOAD LENGTH
                std::size_t payload_length = message.length();
                // bool use_mask = true;  // Always mask for client messages

                if (payload_length < 126) {
                    frame.push_back(0x80 | static_cast<uint8_t>(payload_length));  // Set mask bit
                } else if (payload_length <= 0xFFFF) {
                    frame.push_back(0x80 | 126);  // Set mask bit + 2-byte extended length
                    frame.push_back(static_cast<uint8_t>((payload_length >> 8) & 0xFF));
                    frame.push_back(static_cast<uint8_t>(payload_length & 0xFF));
                } else {
                    frame.push_back(0x80 | 127);  // Set mask bit + 8-byte extended length
                    for (int i = 7; i >= 0; --i) {
                        frame.push_back(static_cast<uint8_t>((payload_length >> (8 * i)) & 0xFF));
                    }
                }

                // Generate random masking key (in practice, use proper random generation)
                std::array<uint8_t, 4> masking_key = {0x12, 0x34, 0x56, 0x78};
                for (const auto& key : masking_key) {
                    frame.push_back(key);
                }

                // Add masked payload data
                for (std::size_t i = 0; i < message.length(); ++i) {
                    uint8_t byte = static_cast<uint8_t>(message[i]);
                    frame.push_back(byte ^ masking_key[i % 4]);
                }

                return frame;
            }

            // Send method - creates WebSocket frame from string
            static std::vector<uint8_t> send(const std::string& msg) { return createFrame(msg); }

            // Send method with explicit parameters
            static std::vector<uint8_t> send(const std::string& msg, bool fin, uint8_t opcode) {
                return createFrame(msg, fin, opcode);
            }

            // Send masked message (for client-to-server communication)
            static std::vector<uint8_t> sendMasked(const std::string& msg) { return createMaskedFrame(msg); }
        };
    }  // namespace ws
}  // namespace http
#endif