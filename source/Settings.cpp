#include <Settings.hpp>
#include <cstdio>
#include <sys/stat.h>

namespace {

    constexpr const char *SettingsDir = "sdmc:/switch/nxmanga";
    constexpr const char *SettingsPath = "sdmc:/switch/nxmanga/settings.cfg";

    bool g_loaded = false;
    settings::ReadingOrientation g_reading_orientation = settings::ReadingOrientation::Horizontal;
    std::string g_language = "";
    bool g_cascade_mode = false;

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

        // "-" stands in for an empty language, since %s can't match a blank
        // token and would otherwise misalign onto the next field (cascade).
        char language_buf[32] = {};
        if (std::fscanf(file, "%31s", language_buf) == 1) {
            g_language = (std::string(language_buf) == "-") ? "" : language_buf;
        }

        int cascade_value = 0;
        if (std::fscanf(file, "%d", &cascade_value) == 1) {
            g_cascade_mode = (cascade_value == 1);
        }
        std::fclose(file);
    }

    void Save() {
        mkdir(SettingsDir, 0777);
        auto file = std::fopen(SettingsPath, "w");
        if (file == nullptr) {
            return;
        }
        const auto language_field = g_language.empty() ? "-" : g_language.c_str();
        std::fprintf(file, "%d\n%s\n%d", (g_reading_orientation == settings::ReadingOrientation::Vertical) ? 1 : 0, language_field, g_cascade_mode ? 1 : 0);
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

    bool GetCascadeMode() {
        EnsureLoaded();
        return g_cascade_mode;
    }

    void SetCascadeMode(const bool enabled) {
        EnsureLoaded();
        if (g_cascade_mode == enabled) {
            return;
        }
        g_cascade_mode = enabled;
        Save();
    }

}
