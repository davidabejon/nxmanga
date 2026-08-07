#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace manga {

    // A MangaSource exposes a manga/chapter as an ordered list of encoded page
    // images, regardless of how they are actually stored (loose files in a
    // directory, entries inside an archive, ...). MangaViewerLayout only ever
    // talks to this interface, so adding a new storage format never touches it.
    class IMangaSource {
        public:
            virtual ~IMangaSource() = default;

            virtual size_t GetPageCount() const = 0;

            // Returns the raw encoded bytes (jpg/png/...) for the page at index,
            // ready to be handed to pu::ui::render::LoadImageFromBuffer. Returns
            // an empty vector if the page is out of range or could not be read.
            virtual std::vector<uint8_t> ReadPage(const size_t index) const = 0;
    };

    using MangaSourcePtr = std::unique_ptr<IMangaSource>;

}
