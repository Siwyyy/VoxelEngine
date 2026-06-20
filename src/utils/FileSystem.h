#pragma once

#include <filesystem>
#include <string>
#include <vector>

class FileSystem
{
public:
    [[nodiscard]] static std::filesystem::path getExecutableDir();
    [[nodiscard]] static std::vector<char> readFile(const std::string& relativePath);
};
