#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

#include "utils.h"
#ifdef HTTP_OPENSSL_SUPPORT
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "logger.h"

namespace http {

    /**
     * @brief Enum representing the result of WebSocket operations
     *
     * This enum is used to indicate the type of data received from a WebSocket connection
     * or the success/failure of operations.
     */
    enum class Result : int { Fail = 0, Text = 1, Binary = 2, Ping = 3, Pong = 4 };

    /**
     * @brief Enum representing WebSocket opcodes
     *
     * WebSocket opcodes define the type of frame being sent or received.
     */
    enum class WsOpcode : uint8_t {
        Continuation = 0x0,
        Text = 0x1,
        Binary = 0x2,
        Close = 0x8,
        Ping = 0x9,
        Pong = 0xA,
    };

    /**
     * @brief Structure representing a WebSocket frame
     *
     * This structure holds all the information about a WebSocket frame including
     * control flags, opcode, payload length, masking key, and payload data.
     */
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

    /**
     * @brief WebSocket class for handling WebSocket connections
     *
     * This class provides methods for creating, reading, and writing WebSocket frames,
     * as well as managing the connection lifecycle.
     */
    class WebSocket {
       private:
        // logger
        Logger& log = Logger::getInstance();

       public:
        /**
         * @brief Construct a new WebSocket object
         *
         * @param sockfd Socket file descriptor for the connection
         * @param raw_request Raw HTTP request string
         * @param query Query parameters from the connection
         */
        WebSocket(int sockfd, const std::string& raw_request, const std::unordered_map<std::string, std::string> query)
            : frame{sockfd}, byte_data(raw_request.begin(), raw_request.end()), query_(query), isOpen(true) {
            log.debug("Websocket open FD={}", sockfd);
        }
#ifdef HTTP_OPENSSL_SUPPORT
        /**
         * @brief Construct a new WebSocket object with SSL support
         *
         * @param sockfd Socket file descriptor for the connection
         * @param ssl SSL context for encrypted connection
         * @param raw_request Raw HTTP request string
         * @param query Query parameters from the connection
         */
        WebSocket(int sockfd, SSL* ssl, const std::string& raw_request,
                  const std::unordered_map<std::string, std::string> query)
            : frame{sockfd, ssl}, byte_data(raw_request.begin(), raw_request.end()), query_(query), isOpen(true) {}
#endif

        /**
         * @brief Get the current WebSocket frame
         *
         * @return Frame object representing the current frame state
         */
        Frame getFrame() { return frame; }

        /**
         * @brief Get query parameters
         *
         * @return Reference to the query parameter map
         */
        const std::unordered_map<std::string, std::string>& query() const { return query_; }

        /**
         * @brief Close the WebSocket connection
         *
         * This method closes the underlying socket and cleans up SSL resources if needed.
         */
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

        /**
         * @brief Send a message through the WebSocket connection
         *
         * @param msg Message to send
         * @return Number of bytes sent, or -1 on error
         */
        ssize_t send(const std::string& msg) {
            if (!utils::isSocketAlive(frame.sockfd)) {
                // close();
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

        /**
         * @brief Read a message from the WebSocket connection
         *
         * @param msg Reference to string to store the received message
         * @return Result indicating the type of data received or failure
         */
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
            switch (frame.opcode) {
                case WsOpcode::Text:
                    return Result::Text;
                case WsOpcode::Binary:
                    return Result::Binary;
                case WsOpcode::Ping:
                    return Result::Ping;  // Add this line
                case WsOpcode::Pong:
                    return Result::Pong;  // Add this line
                default:
                    return Result::Fail;
            }
        }

        /**
         * @brief Operator for reading from WebSocket
         *
         * @param ws WebSocket object to read from
         * @param msg Reference to string to store the message
         * @return Result of the read operation
         */
        friend Result operator>>(WebSocket& ws, std::string& msg) { return ws.read(msg); }

        /**
         * @brief Operator for writing to WebSocket
         *
         * @param ws WebSocket object to write to
         * @param msg Message to send
         * @return Number of bytes sent
         */
        // For sending messages - returns ssize_t like send() does
        friend ssize_t operator<<(WebSocket& ws, const std::string& msg) { return ws.send(msg); }

       private:
        Frame frame;
        std::vector<uint8_t> byte_data;
        std::unordered_map<std::string, std::string> query_;
        bool isOpen = false;

        /**
         * @brief Read a single WebSocket frame from raw bytes
         *
         * @param data Raw bytes to parse into a frame
         * @return true if parsing was successful, false otherwise
         */
        bool readFrame(const std::vector<uint8_t>& data);

        /**
         * @brief Create a WebSocket frame for sending
         *
         * @param message Message to include in the frame
         * @param fin Final frame flag
         * @param opcode Opcode for the frame
         * @param mask Whether to apply masking
         * @return Vector of bytes representing the complete frame
         */
        std::vector<uint8_t> writeFrame(const std::string& message, bool fin, WsOpcode opcode, bool mask = false);
    };

    /**
     * @brief Get WebSocket frame type from raw data
     *
     * @param data Raw data to analyze
     * @return Optional containing the detected opcode, or nullopt if invalid
     */
    std::optional<WsOpcode> getWebSocketFrame(const std::string& data);

    /**
     * @brief Convert WebSocket opcode to string representation
     *
     * @param opcode Opcode to convert
     * @return String representation of the opcode
     */
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