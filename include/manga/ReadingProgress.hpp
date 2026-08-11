#pragma once

#include <cstdint>
#include <string>

namespace manga {

    // A manga/chapter's last known reading position. page_count is stored
    // alongside current_page (rather than a separate "completed" flag) so
    // completion can always be recomputed against the most recently saved
    // total, without needing to reopen the manga source just to check it.
    struct ReadingProgress {
        uint32_t current_page = 0;
        size_t page_count = 0;
    };

    // Returns the saved progress for path, or a default (page 0, no known
    // page count) if none has been saved yet.
    ReadingProgress GetProgress(const std::string &path);

    // Saves current_page/page_count as path's reading progress. Does
    // nothing if the saved value is already exactly this.
    void SaveProgress(const std::string &path, const uint32_t current_page, const size_t page_count);

    // True if path has saved progress and it reaches its last page.
    bool IsCompleted(const std::string &path);

    // True if path (a leaf manga, or a folder of further manga entries) has
    // been fully read: for a leaf, equivalent to IsCompleted; for a folder,
    // true only if every entry under it is also fully read. False for an
    // empty folder, since nothing in it has been read.
    bool IsFullyRead(const std::string &path);

}
