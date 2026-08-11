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

    // Aggregate reading state for path (a leaf manga, or a folder of further
    // manga entries). For a leaf, this is just its own completion/progress.
    // For a folder, it's derived from every entry under it: Completed only
    // if all of them are; NotStarted only if none of them have been opened
    // (also true for an empty folder); InProgress otherwise.
    enum class ReadStatus {
        NotStarted,
        InProgress,
        Completed
    };

    ReadStatus GetReadStatus(const std::string &path);

    // Forces path to be considered fully read: for a leaf, seeks it to its
    // last page (opening its source to learn the page count, if it hasn't
    // been opened before); for a folder, does this for every entry under it.
    void MarkAsRead(const std::string &path);

    // Clears path's saved progress, resetting it back to never opened: for
    // a leaf, clears just its own; for a folder, every entry under it.
    void MarkAsUnread(const std::string &path);

}
