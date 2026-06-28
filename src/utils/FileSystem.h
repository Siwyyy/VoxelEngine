#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace voxl
{
    class FileSystem
    {
    public:
        [[nodiscard]] static std::filesystem::path getExecutableDir();
        [[nodiscard]] static std::vector<char> readFile(const std::string& relativePath);
    };
} // namespace voxl
