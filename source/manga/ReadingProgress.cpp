#include <manga/ReadingProgress.hpp>
#include <manga/PathHash.hpp>
#include <manga/MangaSource.hpp>
#include <cstdio>
#include <unordered_map>
#include <sys/stat.h>

namespace manga {

    namespace {

        constexpr const char *ProgressDirPath = "sdmc:/switch/nxmanga/progress";

        std::unordered_map<std::string, ReadingProgress> g_MemoryCache;
        bool g_ProgressDirReady = false;

        std::string GetProgressFilePath(const std::string &path) {
            return std::string(ProgressDirPath) + "/" + HashPath(path) + ".progress";
        }

        void EnsureProgressDirExists() {
            if (g_ProgressDirReady) {
                return;
            }

            mkdir("sdmc:/switch", 0777);
            mkdir("sdmc:/switch/nxmanga", 0777);
            mkdir(ProgressDirPath, 0777);
            g_ProgressDirReady = true;
        }

        ReadingProgress ReadProgressFile(const std::string &path) {
            ReadingProgress progress;

            auto file = std::fopen(GetProgressFilePath(path).c_str(), "r");
            if (file == nullptr) {
                return progress;
            }

            uint32_t current_page = 0;
            size_t page_count = 0;
            if (std::fscanf(file, "%u %zu", &current_page, &page_count) == 2) {
                progress = {current_page, page_count};
            }
            std::fclose(file);
            return progress;
        }

        void ClearProgress(const std::string &path) {
            g_MemoryCache.erase(path);
            std::remove(GetProgressFilePath(path).c_str());
        }

    }

    ReadingProgress GetProgress(const std::string &path) {
        const auto it = g_MemoryCache.find(path);
        if (it != g_MemoryCache.end()) {
            return it->second;
        }

        const auto progress = ReadProgressFile(path);
        g_MemoryCache.emplace(path, progress);
        return progress;
    }

    void SaveProgress(const std::string &path, const uint32_t current_page, const size_t page_count) {
        const auto it = g_MemoryCache.find(path);
        if ((it != g_MemoryCache.end()) && (it->second.current_page == current_page) && (it->second.page_count == page_count)) {
            return;
        }

        g_MemoryCache[path] = {current_page, page_count};

        EnsureProgressDirExists();
        auto file = std::fopen(GetProgressFilePath(path).c_str(), "w");
        if (file == nullptr) {
            return;
        }
        std::fprintf(file, "%u\n%zu", current_page, page_count);
        std::fclose(file);
    }

    bool IsCompleted(const std::string &path) {
        const auto progress = GetProgress(path);
        return (progress.page_count > 0) && ((progress.current_page + 1) >= progress.page_count);
    }

    ReadStatus GetReadStatus(const std::string &path) {
        if (IsLeafManga(path)) {
            if (IsCompleted(path)) {
                return ReadStatus::Completed;
            }
            return (GetProgress(path).page_count > 0) ? ReadStatus::InProgress : ReadStatus::NotStarted;
        }

        const auto entries = ListMangaEntries(path);
        if (entries.empty()) {
            return ReadStatus::NotStarted;
        }

        auto all_completed = true;
        auto any_started = false;
        for (const auto &name : entries) {
            const auto child_status = GetReadStatus(path + "/" + name);
            if (child_status != ReadStatus::Completed) {
                all_completed = false;
            }
            if (child_status != ReadStatus::NotStarted) {
                any_started = true;
            }
        }

        if (all_completed) {
            return ReadStatus::Completed;
        }
        return any_started ? ReadStatus::InProgress : ReadStatus::NotStarted;
    }

    void MarkAsRead(const std::string &path) {
        if (IsLeafManga(path)) {
            auto page_count = GetProgress(path).page_count;
            if (page_count == 0) {
                const auto source = OpenMangaSource(path);
                if (source == nullptr) {
                    return;
                }
                page_count = source->GetPageCount();
            }
            if (page_count == 0) {
                return;
            }
            SaveProgress(path, static_cast<uint32_t>(page_count - 1), page_count);
            return;
        }

        for (const auto &name : ListMangaEntries(path)) {
            MarkAsRead(path + "/" + name);
        }
    }

    void MarkAsUnread(const std::string &path) {
        if (IsLeafManga(path)) {
            ClearProgress(path);
            return;
        }

        for (const auto &name : ListMangaEntries(path)) {
            MarkAsUnread(path + "/" + name);
        }
    }

}
