#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace utils {

    // Compresses raw data using zlib format (miniz)
    std::vector<uint8_t> compress(const std::string& data);

    // Decompresses zlib-compressed bytes back into an uncompressed string
    std::string decompress(const std::vector<uint8_t>& compressed_data);

    // Computes the standard 40-character hex SHA-1 digest
    std::string sha1(const std::string& data);

    // Converts 40-char hex string to 20-byte binary string
    std::string hex_to_bytes(const std::string& hex);

    // Converts 20-byte binary string to 40-char hex string
    std::string bytes_to_hex(const std::string& bytes);

} // namespace utils