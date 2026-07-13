#pragma once

#include <fstream>
#include <string>
#include <sstream>

namespace utils {

    inline std::filesystem::path asset_path(
        const std::string &filepath) {
        return  std::filesystem::path(ASSET_ROOT) / filepath;
    }

    inline std::string read_file(const std::string& filepath) {
        std::ifstream file(filepath);
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
}