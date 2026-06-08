#pragma once
#include <filesystem>
#include <fstream>
#include <string>

inline std::string writeTempScript(const std::string& prefix, const std::string& content) {
    namespace fs = std::filesystem;
    static int counter = 0;
    fs::path p = fs::temp_directory_path() /
                 ("codefab_" + prefix + "_" + std::to_string(++counter) + ".txt");
    std::ofstream(p) << content;
    return p.string();
}

inline bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}
