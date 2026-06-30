#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace voxl
{
    class FileSystem
    {
    public:
        [[nodiscard]] static std::filesystem::path getExecutableDir();
        [[nodiscard]] static std::expected<std::vector<char>, std::string> readFile(const std::string& relativePath);
    };
} // namespace voxl
