#pragma once

#include <vector>
#include <string>
#include <filesystem>

class FileSystem
{
public:
    [[nodiscard]] static std::filesystem::path getExecutableDir();
    [[nodiscard]] static std::vector<char> readFile(const std::string& relativePath);
};
