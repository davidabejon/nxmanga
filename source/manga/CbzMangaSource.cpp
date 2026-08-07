#include <manga/CbzMangaSource.hpp>
#include <FsUtils.hpp>
#include <minizip/unzip.h>
#include <algorithm>

namespace manga {

    struct CbzMangaSource::Impl {
        unzFile file = nullptr;
        std::vector<std::string> page_names;

        ~Impl() {
            if (this->file != nullptr) {
                unzClose(this->file);
            }
        }
    };

    CbzMangaSource::CbzMangaSource(const std::string &path) : impl(std::make_unique<Impl>()) {
        this->impl->file = unzOpen(path.c_str());
        if (this->impl->file == nullptr) {
            return;
        }

        if (unzGoToFirstFile(this->impl->file) != UNZ_OK) {
            return;
        }

        std::vector<std::string> names;
        do {
            char name_buf[512];
            if (unzGetCurrentFileInfo(this->impl->file, nullptr, name_buf, sizeof(name_buf), nullptr, 0, nullptr, 0) != UNZ_OK) {
                continue;
            }

            const std::string name(name_buf);
            if (fs::HasImageExtension(name)) {
                names.push_back(name);
            }
        } while (unzGoToNextFile(this->impl->file) == UNZ_OK);

        std::sort(names.begin(), names.end());
        this->impl->page_names = std::move(names);
    }

    CbzMangaSource::~CbzMangaSource() = default;

    size_t CbzMangaSource::GetPageCount() const {
        return this->impl->page_names.size();
    }

    std::vector<uint8_t> CbzMangaSource::ReadPage(const size_t index) const {
        if ((this->impl->file == nullptr) || (index >= this->impl->page_names.size())) {
            return {};
        }

        if (unzLocateFile(this->impl->file, this->impl->page_names.at(index).c_str(), 0) != UNZ_OK) {
            return {};
        }

        unz_file_info info;
        if (unzGetCurrentFileInfo(this->impl->file, &info, nullptr, 0, nullptr, 0, nullptr, 0) != UNZ_OK) {
            return {};
        }

        if (unzOpenCurrentFile(this->impl->file) != UNZ_OK) {
            return {};
        }

        std::vector<uint8_t> data(info.uncompressed_size);
        const auto read = unzReadCurrentFile(this->impl->file, data.data(), static_cast<unsigned>(data.size()));
        unzCloseCurrentFile(this->impl->file);

        if (read != static_cast<int>(data.size())) {
            return {};
        }

        return data;
    }

}
