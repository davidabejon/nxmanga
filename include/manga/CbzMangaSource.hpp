#pragma once

#include <manga/IMangaSource.hpp>
#include <memory>
#include <string>

namespace manga {

    // A manga stored as a CBZ (a zip archive of loose image files).
    // The zip container is opened lazily and kept open for the lifetime of the
    // source so individual pages can be extracted on demand.
    class CbzMangaSource : public IMangaSource {
        public:
            explicit CbzMangaSource(const std::string &path);
            ~CbzMangaSource() override;

            size_t GetPageCount() const override;
            std::vector<uint8_t> ReadPage(const size_t index) const override;

        private:
            struct Impl;
            std::unique_ptr<Impl> impl;
    };

}
