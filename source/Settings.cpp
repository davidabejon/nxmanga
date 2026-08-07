#include <Settings.hpp>
#include <cstdio>
#include <sys/stat.h>

namespace {

    constexpr const char *SettingsDir = "sdmc:/switch/nxmanga";
    constexpr const char *SettingsPath = "sdmc:/switch/nxmanga/settings.cfg";

    bool g_loaded = false;
    settings::ReadingOrientation g_reading_orientation = settings::ReadingOrientation::Horizontal;
    std::string g_language = "";

    void EnsureLoaded() {
        if (g_loaded) {
            return;
        }
        g_loaded = true;

        auto file = std::fopen(SettingsPath, "r");
        if (file == nullptr) {
            return;
        }

        int value = 0;
        if (std::fscanf(file, "%d", &value) == 1) {
            g_reading_orientation = (value == 1) ? settings::ReadingOrientation::Vertical : settings::ReadingOrientation::Horizontal;
        }

        char language_buf[32] = {};
        if (std::fscanf(file, "%31s", language_buf) == 1) {
            g_language = language_buf;
        }
        std::fclose(file);
    }

    void Save() {
        mkdir(SettingsDir, 0777);
        auto file = std::fopen(SettingsPath, "w");
        if (file == nullptr) {
            return;
        }
        std::fprintf(file, "%d\n%s", (g_reading_orientation == settings::ReadingOrientation::Vertical) ? 1 : 0, g_language.c_str());
        std::fclose(file);
    }

}

namespace settings {

    ReadingOrientation GetReadingOrientation() {
        EnsureLoaded();
        return g_reading_orientation;
    }

    void SetReadingOrientation(const ReadingOrientation orientation) {
        EnsureLoaded();
        if (g_reading_orientation == orientation) {
            return;
        }
        g_reading_orientation = orientation;
        Save();
    }

    std::string GetLanguage() {
        EnsureLoaded();
        return g_language;
    }

    void SetLanguage(const std::string &language_code) {
        EnsureLoaded();
        if (g_language == language_code) {
            return;
        }
        g_language = language_code;
        Save();
    }

}
