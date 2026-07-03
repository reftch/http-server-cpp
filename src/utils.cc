#include "utils.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>  // Must include this for stringstream

namespace utils {

    std::map<std::string, std::string> parseArguments(int argc, char* argv[]) {
        std::map<std::string, std::string> args;

        // Set default values
        args["--host"] = "0.0.0.0";
        args["--port"] = "8080";
        args["--help"] = "false";

        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "--host" && i + 1 < argc) {
                args["--host"] = argv[i + 1];
                i++;
            } else if (arg == "--port" && i + 1 < argc) {
                args["--port"] = argv[i + 1];
                i++;
            } else if (arg == "--help") {
                args["--help"] = "true";
            }
        }

        return args;
    }

    std::string mapToJson(const std::unordered_map<std::string, std::string>& params) {
        std::stringstream ss;
        ss << "{";  // Start the JSON object

        bool first = true;

        // Iterate over all key-value pairs in the map
        for (const auto& pair : params) {
            // Add a comma separator if it's not the first element
            if (!first) {
                ss << ", ";
            }

            // Start the key (must be a quoted string)
            ss << "\"" << pair.first << "\": ";

            // Add the value (must be a quoted string)
            // Note: Since the input is string-based, we wrap the value in quotes.
            ss << "\"" << pair.second << "\"";

            first = false;
        }

        ss << "}";  // End the JSON object

        return ss.str();
    }

    std::string readFile(const std::string& path) {
        // Binary mode for faster reading
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            return {};
        }

        // Get file size and read efficiently
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::string content(size, '\0');
        file.read(content.data(), size);
        return content;
    }

    std::string urlDecode(const std::string& encoded) {
        std::string decoded;
        for (size_t i = 0; i < encoded.length(); ++i) {
            if (encoded[i] == '%' && i + 2 < encoded.length()) {
                // Convert hex to char
                char hex[3] = {encoded[i + 1], encoded[i + 2], '\0'};
                char c = static_cast<char>(strtol(hex, nullptr, 16));
                decoded += c;
                i += 2;
            } else if (encoded[i] == '+') {
                decoded += ' ';
            } else {
                decoded += encoded[i];
            }
        }
        return decoded;
    }

    time_t getMtime(const std::string& path) {
        // Check if file exists
        if (!std::filesystem::exists(path)) {
            return -1;
        }

        // Get the file status
        auto status = std::filesystem::status(path);

        // Check if it's a regular file
        if (!std::filesystem::is_regular_file(status)) {
            return -1;
        }

        // Get the last modification time
        auto mtime = std::filesystem::last_write_time(path);

        // Convert to time_t using std::chrono::duration_cast
        auto time_t_duration = std::chrono::duration_cast<std::chrono::seconds>(mtime.time_since_epoch());

        return static_cast<time_t>(time_t_duration.count());
    }

    std::string fileMtimeToHttpDate(time_t mtime) {
        if (mtime < 0) {
            return std::string();
        }

        struct tm tm_buf;
        if (gmtime_r(&mtime, &tm_buf) == nullptr) {
            return std::string();
        }
        char buf[64];
        if (strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm_buf) == 0) {
            return std::string();
        }

        return std::string(buf);
    }

    std::string computeEtag(size_t mtime, size_t size) {
        // If mtime cannot be determined (negative value indicates an error
        // or sentinel), do not generate an ETag. Returning a neutral / fixed
        // value like 0 could collide with a real file that legitimately has
        // mtime == 0 (epoch) and lead to misleading validators.

        // if (mtime < 0) {
        //     return std::string();
        // }

        return std::string("W/\"") + fromIntToHex(mtime) + "-" + fromIntToHex(size) + "\"";
    }

    std::string fromIntToHex(size_t n) {
        static const auto charset = "0123456789abcdef";
        std::string ret;
        do {
            ret = charset[n & 15] + ret;
            n >>= 4;
        } while (n > 0);
        return ret;
    }

    std::string sha1(const std::string& input) {
        // RFC 3174 SHA-1 implementation
        auto left_rotate = [](uint32_t x, uint32_t n) -> uint32_t {
            return (x << n) | (x >> (32 - n));
        };

        uint32_t h0 = 0x67452301;
        uint32_t h1 = 0xEFCDAB89;
        uint32_t h2 = 0x98BADCFE;
        uint32_t h3 = 0x10325476;
        uint32_t h4 = 0xC3D2E1F0;

        // Pre-processing: adding padding bits
        std::string msg = input;
        uint64_t original_bit_len = static_cast<uint64_t>(msg.size()) * 8;
        msg.push_back(static_cast<char>(0x80u));
        while (msg.size() % 64 != 56) {
            msg.push_back(0);
        }

        // Append original length in bits as 64-bit big-endian
        for (int i = 56; i >= 0; i -= 8) {
            msg.push_back(static_cast<char>((original_bit_len >> i) & 0xFF));
        }

        // Process each 512-bit chunk
        for (size_t offset = 0; offset < msg.size(); offset += 64) {
            uint32_t w[80];

            for (size_t i = 0; i < 16; i++) {
                w[i] = (static_cast<uint32_t>(static_cast<uint8_t>(msg[offset + i * 4])) << 24) |
                       (static_cast<uint32_t>(static_cast<uint8_t>(msg[offset + i * 4 + 1])) << 16) |
                       (static_cast<uint32_t>(static_cast<uint8_t>(msg[offset + i * 4 + 2])) << 8) |
                       (static_cast<uint32_t>(static_cast<uint8_t>(msg[offset + i * 4 + 3])));
            }

            for (int i = 16; i < 80; i++) {
                w[i] = left_rotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
            }

            uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;

            for (int i = 0; i < 80; i++) {
                uint32_t f, k;
                if (i < 20) {
                    f = (b & c) | ((~b) & d);
                    k = 0x5A827999;
                } else if (i < 40) {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                } else if (i < 60) {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDC;
                } else {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }

                uint32_t temp = left_rotate(a, 5) + f + e + k + w[i];
                e = d;
                d = c;
                c = left_rotate(b, 30);
                b = a;
                a = temp;
            }

            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
        }

        // Produce the final hash as a 20-byte binary string
        std::string hash(20, '\0');
        for (size_t i = 0; i < 4; i++) {
            hash[i] = static_cast<char>((h0 >> (24 - i * 8)) & 0xFF);
            hash[4 + i] = static_cast<char>((h1 >> (24 - i * 8)) & 0xFF);
            hash[8 + i] = static_cast<char>((h2 >> (24 - i * 8)) & 0xFF);
            hash[12 + i] = static_cast<char>((h3 >> (24 - i * 8)) & 0xFF);
            hash[16 + i] = static_cast<char>((h4 >> (24 - i * 8)) & 0xFF);
        }
        return hash;
    }

    std::string base64_encode(const std::string& input) {
        static const auto lookup = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string out;
        out.reserve(input.size());

        auto val = 0;
        auto valb = -6;

        for (auto c : input) {
            val = (val << 8) + static_cast<uint8_t>(c);
            valb += 8;
            while (valb >= 0) {
                out.push_back(lookup[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }

        if (valb > -6) {
            out.push_back(lookup[((val << 8) >> (valb + 8)) & 0x3F]);
        }

        while (out.size() % 4) {
            out.push_back('=');
        }

        return out;
    }

    bool isSocketAlive(int sockfd) {
        if (sockfd < 0) return false;

        int error = 0;
        socklen_t len = sizeof(error);
        int result = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);

        // If getsockopt fails or there's an error, socket is likely closed
        if (result < 0 || error != 0) {
            return false;
        }

        return true;
    }

    std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return "";
        }

        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    bool parseHexSize(const std::string& s, size_t& out) {
        out = 0;

        for (char c : s) {
            out <<= 4;

            if (c >= '0' && c <= '9')
                out += c - '0';
            else if (c >= 'a' && c <= 'f')
                out += c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                out += c - 'A' + 10;
            else
                return false;
        }

        return true;
    }

}  // namespace utils
