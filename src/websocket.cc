#include "websocket.h"

#include <string>
namespace http {

    bool WebSocket::readFrame(const std::vector<uint8_t>& data) {
        std::size_t offset = 0;

        if (data.size() < 2) {
            return false;  // Invalid frame
        }

        // Read FIN and RSV bits (1 byte)
        uint8_t first_byte = data[offset++];
        frame.fin = (first_byte & 0x80) != 0;
        frame.opcode = static_cast<WsOpcode>(first_byte & 0x0F);

        if (frame.opcode == WsOpcode::Close) {
            return false;
        }

        // Read mask and payload length (1 byte)
        uint8_t second_byte = data[offset++];
        frame.mask = (second_byte & 0x80) != 0;
        frame.payload_length = second_byte & 0x7F;

        // Handle extended payload length
        if (frame.payload_length == 126) {
            // 2-byte extended length
            if (data.size() < offset + 2) return false;  // Incomplete frame
            frame.payload_length = (data[offset] << 8) | data[offset + 1];
            offset += 2;
        } else if (frame.payload_length == 127) {
            // 8-byte extended length
            if (data.size() < offset + 8) return false;  // Incomplete frame
            frame.payload_length = 0;
            for (int i = 0; i < 8; ++i) {
                frame.payload_length |= (static_cast<std::size_t>(data[offset + i]) << (56 - 8 * i));
            }
            offset += 8;
        }

        // Read masking key if present
        if (frame.mask) {
            if (data.size() < offset + 4) return false;  // Incomplete frame
            for (int i = 0; i < 4; ++i) {
                frame.masking_key[i] = data[offset + i];
            }
            offset += 4;
        }

        // Read payload data
        if (data.size() < offset + frame.payload_length) return false;  // Incomplete frame

        frame.payload_data.resize(frame.payload_length);
        for (std::size_t i = 0; i < frame.payload_length; ++i) {
            frame.payload_data[i] = data[offset + i];
            // Unmask if needed
            if (frame.mask) {
                frame.payload_data[i] ^= frame.masking_key[i % 4];
            }
        }

        // Convert to text if it's a text frame
        if (frame.opcode == WsOpcode::Text) {
            frame.text_payload = std::string(frame.payload_data.begin(), frame.payload_data.end());
        }

        return true;
    }

    std::vector<uint8_t> WebSocket::writeFrame(const std::string& message, bool fin, WsOpcode opcode, bool mask) {
        std::vector<uint8_t> frame;

        // First byte: FIN bit + RSV bits + OPCODE
        uint8_t first_byte = (fin ? 0x80 : 0x00) | static_cast<uint8_t>(opcode);
        frame.push_back(first_byte);

        // Second byte: MASK bit + PAYLOAD LENGTH
        std::size_t payload_length = message.length();

        if (mask) {
            frame.push_back(0x80 | (payload_length < 126 ? payload_length : (payload_length <= 0xFFFF ? 126 : 127)));
        } else {
            frame.push_back(payload_length < 126 ? static_cast<uint8_t>(payload_length)
                                                 : (payload_length <= 0xFFFF ? 126 : 127));
        }

        if (mask) {
            // Generate random mask key
            uint8_t mask_key[4];
            for (int i = 0; i < 4; ++i) {
                mask_key[i] = static_cast<uint8_t>(rand() % 256);
            }
            for (int i = 0; i < 4; ++i) {
                frame.push_back(mask_key[i]);
            }

            // XOR payload with mask
            for (size_t i = 0; i < message.size(); ++i) {
                uint8_t byte = static_cast<uint8_t>(message[i]);
                byte ^= mask_key[i % 4];
                frame.push_back(byte);
            }
        } else {
            // No mask - just append payload
            for (char c : message) {
                frame.push_back(static_cast<uint8_t>(c));
            }
        }

        return frame;
    }

    bool isWebSocketFrame(const std::string& data) {
        if (data.length() < 2) return false;

        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.data());
        uint8_t first_byte = bytes[0];
        uint8_t opcode = first_byte & 0x0F;
        bool fin = (first_byte & 0x80) != 0;

        // Valid WebSocket opcodes:
        // 0x0 - continuation frame
        // 0x1 - text frame
        // 0x2 - binary frame
        // 0x8 - connection close
        // 0x9 - ping
        // 0xA - pong

        // Valid WebSocket opcodes: 0x0-0xA
        if (opcode > 0xA) return false;

        // Basic frame structure validation
        return fin && opcode <= 0xA;
    }

}  // namespace http