#pragma once

#include <string>

namespace manga {

    // Deterministically hashes key (typically a manga/chapter path) into a
    // 16-hex-digit, filename-safe string, so on-disk caches keyed by path
    // (covers, reading progress, ...) can use it as a stable file name.
    std::string HashPath(const std::string &key);

}
