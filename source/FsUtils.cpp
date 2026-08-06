#include <FsUtils.hpp>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

namespace fs {

    namespace {

        bool HasImageExtension(const std::string &name) {
            const auto dot_pos = name.find_last_of('.');
            if (dot_pos == std::string::npos) {
                return false;
            }

            auto ext = name.substr(dot_pos + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            return (ext == "jpg") || (ext == "jpeg");
        }

    }

    std::vector<std::string> ListDirectories(const std::string &path) {
        std::vector<std::string> dirs;
        auto dir = opendir(path.c_str());
        if (dir == nullptr) {
            return dirs;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            const std::string name = entry->d_name;
            if ((name == ".") || (name == "..")) {
                continue;
            }

            struct stat st;
            const auto full_path = path + "/" + name;
            if ((stat(full_path.c_str(), &st) == 0) && S_ISDIR(st.st_mode)) {
                dirs.push_back(name);
            }
        }
        closedir(dir);

        std::sort(dirs.begin(), dirs.end());
        return dirs;
    }

    std::vector<std::string> ListImageFiles(const std::string &path) {
        std::vector<std::string> files;
        auto dir = opendir(path.c_str());
        if (dir == nullptr) {
            return files;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            const std::string name = entry->d_name;
            if (HasImageExtension(name)) {
                files.push_back(name);
            }
        }
        closedir(dir);

        std::sort(files.begin(), files.end());
        return files;
    }

}
