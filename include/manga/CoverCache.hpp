#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace manga {

    // Cover images are expensive to produce (opening a possibly large CBZ,
    // locating an entry, inflating it) but never change for a given manga
    // path, so they're cached both in memory (for this run) and on disk
    // (sdmc:/switch/nxmanga/cache, for future runs) keyed by that path.

    // Returns the cached cover for key, or an empty vector if there is none.
    std::vector<uint8_t> GetCachedCover(const std::string &key);

    // Stores data as the cached cover for key, in memory and on disk.
    void SetCachedCover(const std::string &key, const std::vector<uint8_t> &data);

}
