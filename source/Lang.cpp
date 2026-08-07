#include <Lang.hpp>
#include <Json.hpp>
#include <FsUtils.hpp>
#include <cstdio>

namespace {

    constexpr const char *DefaultLanguage = "gb";
    constexpr const char *LangDir = "romfs:/lang/";

    std::string g_language = DefaultLanguage;
    json::Value g_translations;
    bool g_loaded = false;

    std::string ReadFile(const std::string &path) {
        auto file = std::fopen(path.c_str(), "rb");
        if (file == nullptr) {
            return "";
        }

        std::fseek(file, 0, SEEK_END);
        const auto size = std::ftell(file);
        std::fseek(file, 0, SEEK_SET);

        std::string data;
        if (size > 0) {
            data.resize(static_cast<size_t>(size));
            std::fread(data.data(), 1, data.size(), file);
        }
        std::fclose(file);
        return data;
    }

    std::vector<std::string> SplitKey(const std::string &key) {
        std::vector<std::string> parts;
        size_t start = 0;
        while (start <= key.size()) {
            const auto dot = key.find('.', start);
            if (dot == std::string::npos) {
                parts.push_back(key.substr(start));
                break;
            }
            parts.push_back(key.substr(start, dot - start));
            start = dot + 1;
        }
        return parts;
    }

    void EnsureLoaded() {
        if (g_loaded) {
            return;
        }
        g_loaded = true;

        // The language code is always a variable concatenated into the
        // path, never a hardcoded file name, so switching languages later
        // only means calling lang::SetLanguage with a different code.
        const auto path = std::string(LangDir) + g_language + ".json";
        g_translations = json::Value::Parse(ReadFile(path));
    }

}

namespace lang {

    std::vector<std::string> GetAvailableLanguages() {
        std::vector<std::string> codes;
        // fs::ListEntries already returns entries sorted, so the cycling
        // order stays deterministic as more <code>.json files get added.
        for (const auto &name : fs::ListEntries("romfs:/lang")) {
            if (fs::GetExtension(name) != "json") {
                continue;
            }
            codes.push_back(name.substr(0, name.size() - 5));
        }
        return codes;
    }

    void SetLanguage(const std::string &language_code) {
        g_language = language_code;
        g_loaded = false;
    }

    std::string GetLanguage() {
        return g_language;
    }

    std::string Get(const std::string &key) {
        EnsureLoaded();

        const json::Value *current = &g_translations;
        for (const auto &part : SplitKey(key)) {
            current = &(*current)[part];
        }

        return current->IsString() ? current->AsString() : key;
    }

    std::string Get(const std::string &key, const std::unordered_map<std::string, std::string> &placeholders) {
        auto text = Get(key);
        for (const auto &entry : placeholders) {
            const auto token = "{" + entry.first + "}";
            size_t pos = 0;
            while ((pos = text.find(token, pos)) != std::string::npos) {
                text.replace(pos, token.size(), entry.second);
                pos += entry.second.size();
            }
        }
        return text;
    }

}
