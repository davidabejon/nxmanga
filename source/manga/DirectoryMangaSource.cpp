#include <manga/DirectoryMangaSource.hpp>
#include <FsUtils.hpp>
#include <fstream>

namespace manga {

    DirectoryMangaSource::DirectoryMangaSource(const std::string &path) : dir_path(path), page_files(fs::ListImageFiles(path)) {}

    size_t DirectoryMangaSource::GetPageCount() const {
        return this->page_files.size();
    }

    std::vector<uint8_t> DirectoryMangaSource::ReadPage(const size_t index) const {
        if (index >= this->page_files.size()) {
            return {};
        }

        std::ifstream file(this->dir_path + "/" + this->page_files.at(index), std::ios::binary | std::ios::ate);
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

}
