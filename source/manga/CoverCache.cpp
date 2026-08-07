#include <manga/CoverCache.hpp>
#include <fstream>
#include <unordered_map>
#include <sys/stat.h>

namespace manga {

    namespace {

        constexpr const char *CacheDirPath = "sdmc:/switch/nxmanga/cache";

        std::unordered_map<std::string, std::vector<uint8_t>> g_MemoryCache;
        bool g_CacheDirReady = false;

        std::string HashKey(const std::string &key) {
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

        std::string GetCacheFilePath(const std::string &key) {
            return std::string(CacheDirPath) + "/" + HashKey(key) + ".cache";
        }

        void EnsureCacheDirExists() {
            if (g_CacheDirReady) {
                return;
            }

            mkdir("sdmc:/switch", 0777);
            mkdir("sdmc:/switch/nxmanga", 0777);
            mkdir(CacheDirPath, 0777);
            g_CacheDirReady = true;
        }

        std::vector<uint8_t> ReadFile(const std::string &path) {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) {
                return {};
            }

            const auto size = file.tellg();
            if (size <= 0) {
                return {};
            }

            std::vector<uint8_t> data(static_cast<size_t>(size));
            file.seekg(0);
            file.read(reinterpret_cast<char *>(data.data()), size);
            return data;
        }

        void WriteFile(const std::string &path, const std::vector<uint8_t> &data) {
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            if (!file) {
                return;
            }
            file.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
        }

    }

    std::vector<uint8_t> GetCachedCover(const std::string &key) {
        const auto it = g_MemoryCache.find(key);
        if (it != g_MemoryCache.end()) {
            return it->second;
        }

        auto data = ReadFile(GetCacheFilePath(key));
        if (!data.empty()) {
            g_MemoryCache.emplace(key, data);
        }
        return data;
    }

    void SetCachedCover(const std::string &key, const std::vector<uint8_t> &data) {
        g_MemoryCache[key] = data;

        EnsureCacheDirExists();
        WriteFile(GetCacheFilePath(key), data);
    }

}
