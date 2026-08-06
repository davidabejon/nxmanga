#pragma once

#include <string>
#include <vector>

namespace fs {

    std::vector<std::string> ListDirectories(const std::string &path);
    std::vector<std::string> ListImageFiles(const std::string &path);

}
