#pragma once

#include <manga/IMangaSource.hpp>
#include <string>
#include <vector>

namespace manga {

    // Opens whatever manga is at path (a directory or a supported archive
    // file) and returns the matching IMangaSource. Returns nullptr if path
    // does not point to a supported manga.
    MangaSourcePtr OpenMangaSource(const std::string &path);

    // Lists the entries directly under root that OpenMangaSource can open:
    // subdirectories and files with a supported archive extension.
    std::vector<std::string> ListMangaEntries(const std::string &root);

    // True if path is directly readable as a page sequence (a directory with
    // loose images in it, or a supported archive) and can be handed to
    // MangaViewerLayout as-is. False means path is a directory that only
    // contains further manga entries (e.g. one archive per chapter) and
    // should be browsed with another MangaListLayout instead.
    bool IsLeafManga(const std::string &path);

    // Returns the raw encoded bytes of a representative cover image for path:
    // its first page if path is a leaf manga, or the first page found by
    // descending into its entries (in listing order) otherwise. Returns an
    // empty vector if no page could be found anywhere under path.
    std::vector<uint8_t> GetCoverImage(const std::string &path);

}
