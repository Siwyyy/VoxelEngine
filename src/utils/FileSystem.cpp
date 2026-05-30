#include "FileSystem.h"
#include <fstream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

std::filesystem::path FileSystem::getExecutableDir()
{
#ifdef _WIN32
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
#else
    // Fallback dla Unix (Linux)
    return std::filesystem::canonical("/proc/self/exe").parent_path();
#endif
}

std::vector<char> FileSystem::readFile(const std::string& relativePath)
{
    std::filesystem::path fullPath = getExecutableDir() / relativePath;

    std::ifstream file(fullPath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + fullPath.string());
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    file.close();

    return buffer;
}
