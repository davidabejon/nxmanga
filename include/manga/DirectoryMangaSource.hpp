#pragma once

#include <manga/IMangaSource.hpp>
#include <string>

namespace manga {

    // A manga stored as a plain directory of loose image files.
    class DirectoryMangaSource : public IMangaSource {
        public:
            explicit DirectoryMangaSource(const std::string &path);

            size_t GetPageCount() const override;
            std::vector<uint8_t> ReadPage(const size_t index) const override;

        private:
            std::string dir_path;
            std::vector<std::string> page_files;
    };

}
