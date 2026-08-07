#pragma once

#include <string>
#include <vector>

namespace fs {

    bool IsDirectory(const std::string &path);
    std::string GetExtension(const std::string &name);
    bool HasImageExtension(const std::string &name);

    std::vector<std::string> ListEntries(const std::string &path);
    std::vector<std::string> ListImageFiles(const std::string &path);

}
