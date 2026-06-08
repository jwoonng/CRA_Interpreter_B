#pragma once
#include <filesystem>
#include <fstream>
#include <string>

// RAII wrapper: destructor removes the temp file automatically.
struct TempScript {
    std::string path;
    explicit TempScript(std::string p) : path(std::move(p)) {}
    ~TempScript() { std::filesystem::remove(path); }
    TempScript(const TempScript&) = delete;
    TempScript& operator=(const TempScript&) = delete;
    TempScript(TempScript&&) = default;
    TempScript& operator=(TempScript&&) = default;
    operator const std::string&() const { return path; }
};

inline TempScript writeTempScript(const std::string& prefix, const std::string& content) {
    namespace fs = std::filesystem;
    static int counter = 0;
    fs::path p = fs::temp_directory_path() /
                 ("codefab_" + prefix + "_" + std::to_string(++counter) + ".txt");
    std::ofstream(p) << content;
    return TempScript(p.string());
}

inline bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}
