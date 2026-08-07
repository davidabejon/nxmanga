#include <FsUtils.hpp>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

namespace fs {

    bool IsDirectory(const std::string &path) {
        struct stat st;
        return (stat(path.c_str(), &st) == 0) && S_ISDIR(st.st_mode);
    }

    std::string GetExtension(const std::string &name) {
        const auto dot_pos = name.find_last_of('.');
        if (dot_pos == std::string::npos) {
            return "";
        }

        auto ext = name.substr(dot_pos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }

    bool HasImageExtension(const std::string &name) {
        const auto ext = GetExtension(name);
        return (ext == "jpg") || (ext == "jpeg") || (ext == "png") || (ext == "webp");
    }

    std::vector<std::string> ListEntries(const std::string &path) {
        std::vector<std::string> entries;
        auto dir = opendir(path.c_str());
        if (dir == nullptr) {
            return entries;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            const std::string name = entry->d_name;
            if ((name == ".") || (name == "..")) {
                continue;
            }
            entries.push_back(name);
        }
        closedir(dir);

        std::sort(entries.begin(), entries.end());
        return entries;
    }

    std::vector<std::string> ListImageFiles(const std::string &path) {
        std::vector<std::string> files;
        for (const auto &name : ListEntries(path)) {
            if (HasImageExtension(name)) {
                files.push_back(name);
            }
        }
        return files;
    }

}
