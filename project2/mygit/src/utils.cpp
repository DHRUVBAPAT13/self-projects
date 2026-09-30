#include "utils.hpp"
#include "miniz.h"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>
#include <vector>

namespace utils {

// Compression Routines (miniz)


std::vector<uint8_t> compress(const std::string& data) {
    mz_ulong src_len = static_cast<mz_ulong>(data.size());
    mz_ulong dest_len = mz_compressBound(src_len);
    std::vector<uint8_t> out(dest_len);

    int status = mz_compress(
        out.data(), 
        &dest_len, 
        reinterpret_cast<const unsigned char*>(data.data()), 
        src_len
    );

    if (status != MZ_OK) {
        throw std::runtime_error("miniz compression failed with error: " + std::to_string(status));
    }

    out.resize(dest_len);
    return out;
}

std::string decompress(const std::vector<uint8_t>& compressed_data) {
    mz_ulong dest_len = static_cast<mz_ulong>(compressed_data.size() * 4);
    if (dest_len < 256) dest_len = 256;

    std::vector<uint8_t> buffer;

    while (true) {
        buffer.resize(dest_len);

        int status = mz_uncompress(
            buffer.data(),
            &dest_len,
            compressed_data.data(),
            static_cast<mz_ulong>(compressed_data.size())
        );

        if (status == MZ_OK) {
            return std::string(buffer.begin(), buffer.begin() + dest_len);
        } else if (status == MZ_BUF_ERROR) {
            dest_len *= 2;
        } else {
            throw std::runtime_error("miniz decompression failed with error: " + std::to_string(status));
        }
    }
}

// -------------------------------------------------------------
// Verified Big-Endian SHA-1 Implementation
// -------------------------------------------------------------

namespace {
    inline uint32_t left_rotate(uint32_t value, size_t count) {
        return (value << count) | (value >> (32 - count));
    }
}

std::string sha1(const std::string& data) {
    // Standard SHA-1 initial state constants
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    // 1. Pre-processing: Padding bits
    uint64_t original_bit_len = static_cast<uint64_t>(data.size()) * 8;
    std::vector<uint8_t> msg(data.begin(), data.end());

    // Append '1' bit (0x80)
    msg.push_back(0x80);

    // Append 0x00 padding until msg size is congruent to 56 mod 64
    while ((msg.size() % 64) != 56) {
        msg.push_back(0x00);
    }

    // Append original length as 64-bit big-endian integer
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<uint8_t>((original_bit_len >> (i * 8)) & 0xFF));
    }

    // 2. Process message in successive 512-bit (64-byte) chunks
    for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        uint32_t w[80];

        // Break chunk into sixteen 32-bit big-endian words
        for (int i = 0; i < 16; ++i) {
            size_t idx = chunk + (i * 4);
            w[i] = (static_cast<uint32_t>(msg[idx]) << 24) |
                   (static_cast<uint32_t>(msg[idx + 1]) << 16) |
                   (static_cast<uint32_t>(msg[idx + 2]) << 8) |
                   (static_cast<uint32_t>(msg[idx + 3]));
        }

        // Extend sixteen words into eighty 32-bit words
        for (int i = 16; i < 80; ++i) {
            w[i] = left_rotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        // Initialize working variables
        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;

        // Main compression loop
        for (int i = 0; i < 80; ++i) {
            uint32_t f = 0;
            uint32_t k = 0;

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

        // Add this chunk's hash to result so far
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    // 3. Produce final 40-character hex string
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::setw(8) << h0
       << std::setw(8) << h1
       << std::setw(8) << h2
       << std::setw(8) << h3
       << std::setw(8) << h4;

    return ss.str();
}

std::string hex_to_bytes(const std::string& hex){

    if (hex.size() != 40) {
        throw std::runtime_error("Invalid hex SHA length for conversion: " + hex);
    }

    std::string bytes;
    bytes.reserve(20);

    for (size_t i = 0; i < 40; i += 2) {
        uint8_t byte = static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16));
        bytes.push_back(static_cast<char>(byte));
    }

    return bytes;
}

std::string bytes_to_hex(const std::string& bytes){

    if (bytes.size() != 20) {
        throw std::runtime_error("Invalid binary SHA length for conversion");
    }

    std::ostringstream ss;
    ss << std::hex << std::setfill('0');

    for (unsigned char c : bytes) {
        ss << std::setw(2) << static_cast<int>(c);
    }

    return ss.str();
}

} // namespace utils

