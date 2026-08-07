#pragma once

namespace settings {

    enum class ReadingOrientation {
        Horizontal,
        Vertical
    };

    ReadingOrientation GetReadingOrientation();
    void SetReadingOrientation(const ReadingOrientation orientation);

}
