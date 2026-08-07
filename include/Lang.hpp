#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace lang {

    // Every language code with a romfs:/lang/<code>.json file, sorted for a
    // deterministic cycling order. Lets a language picker discover options
    // by just dropping in more files, without any code change.
    std::vector<std::string> GetAvailableLanguages();

    // Sets the active language code (e.g. "es") and marks the translations
    // as needing a reload, so the next Get() call picks up
    // romfs:/lang/<language_code>.json. The code is always concatenated
    // into that path at runtime; it is never a hardcoded filename, so
    // switching language later only means calling this with a different
    // code.
    void SetLanguage(const std::string &language_code);
    std::string GetLanguage();

    // Looks up a dot-separated key path (e.g. "manga_viewer.no_images") in
    // the active language file, loading it on first use. Returns the key
    // itself if the translation is missing, so a gap is visible instead of
    // silently blank.
    std::string Get(const std::string &key);

    // Same as Get, but replaces "{name}" placeholders in the result with
    // the given values.
    std::string Get(const std::string &key, const std::unordered_map<std::string, std::string> &placeholders);

}
