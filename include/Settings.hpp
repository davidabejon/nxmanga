#pragma once

#include <string>

namespace settings {

    enum class ReadingOrientation {
        Horizontal,
        Vertical
    };

    ReadingOrientation GetReadingOrientation();
    void SetReadingOrientation(const ReadingOrientation orientation);

    // Empty until SetLanguage is called for the first time, meaning no
    // language has been explicitly chosen yet, so callers should leave
    // whatever default the active Lang module already started with.
    std::string GetLanguage();
    void SetLanguage(const std::string &language_code);

}
