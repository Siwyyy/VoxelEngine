#include "FileSystem.h"

#include <array>
#include <expected>
#include <format>
#include <fstream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

namespace voxl
{
    std::filesystem::path FileSystem::getExecutableDir()
    {
#ifdef _WIN32
        std::array<wchar_t, MAX_PATH> path{};
        GetModuleFileNameW(nullptr, path.data(), path.size());
        return std::filesystem::path(path.data()).parent_path();
#else
        // Fallback dla Unix (Linux)
        return std::filesystem::canonical("/proc/self/exe").parent_path();
#endif
    }

    std::expected<std::vector<char>, std::string> FileSystem::readFile(const std::string& relativePath)
    {
        const std::filesystem::path fullPath = getExecutableDir() / relativePath;

        std::ifstream file(fullPath, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            return std::unexpected(std::format("Failed to open file: {}", fullPath.string()));
        }

        const size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
        file.close();

        return buffer;
    }
} // namespace voxl
