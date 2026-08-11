#include <manga/PathHash.hpp>
#include <cstdint>

namespace manga {

    std::string HashPath(const std::string &key) {
        uint64_t hash = 1469598103934665603ULL; // FNV-1a 64-bit offset basis
        for (const auto c : key) {
            hash ^= static_cast<uint8_t>(c);
            hash *= 1099511628211ULL; // FNV-1a 64-bit prime
        }

        static const char digits[] = "0123456789abcdef";
        std::string result(16, '0');
        for (int i = 15; i >= 0; i--) {
            result[static_cast<size_t>(i)] = digits[hash & 0xF];
            hash >>= 4;
        }
        return result;
    }

}
