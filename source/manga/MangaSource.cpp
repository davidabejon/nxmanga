#include <manga/MangaSource.hpp>
#include <manga/DirectoryMangaSource.hpp>
#include <manga/CbzMangaSource.hpp>
#include <FsUtils.hpp>
#include <unordered_map>

namespace manga {

    namespace {

        using ArchiveFactory = MangaSourcePtr (*)(const std::string &);

        MangaSourcePtr OpenCbzSource(const std::string &path) {
            return std::make_unique<CbzMangaSource>(path);
        }

        // Extension -> factory for every supported archive format. Adding
        // support for a new archive-based format (cbr, cb7, ...) only needs a
        // new IMangaSource implementation and an entry here.
        const std::unordered_map<std::string, ArchiveFactory> &GetArchiveFactories() {
            static const std::unordered_map<std::string, ArchiveFactory> factories = {
                {"cbz", OpenCbzSource},
                {"zip", OpenCbzSource},
            };
            return factories;
        }

        bool IsSupportedArchive(const std::string &name) {
            const auto &factories = GetArchiveFactories();
            return factories.find(fs::GetExtension(name)) != factories.end();
        }

    }

    MangaSourcePtr OpenMangaSource(const std::string &path) {
        if (fs::IsDirectory(path)) {
            return std::make_unique<DirectoryMangaSource>(path);
        }

        const auto &factories = GetArchiveFactories();
        const auto it = factories.find(fs::GetExtension(path));
        if (it == factories.end()) {
            return nullptr;
        }
        return it->second(path);
    }

    std::vector<std::string> ListMangaEntries(const std::string &root) {
        std::vector<std::string> entries;
        for (const auto &name : fs::ListEntries(root)) {
            if (fs::IsDirectory(root + "/" + name) || IsSupportedArchive(name)) {
                entries.push_back(name);
            }
        }

        return entries;
    }

    bool IsLeafManga(const std::string &path) {
        if (fs::IsDirectory(path)) {
            return !fs::ListImageFiles(path).empty();
        }
        return IsSupportedArchive(path);
    }

}
