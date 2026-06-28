#pragma once
#include <string>

#include <vulkan/vulkan.h>

namespace voxl
{
    class Shader
    {
    public:
        Shader(VkDevice device, const std::string& filePath);
        ~Shader();

        Shader(const Shader&)            = delete;
        Shader& operator=(const Shader&) = delete;

        Shader(Shader&& other) noexcept;
        Shader& operator=(Shader&& other) noexcept;

        [[nodiscard]] VkShaderModule getModule() const { return m_module; }

    private:
        VkDevice m_device;
        VkShaderModule m_module = VK_NULL_HANDLE;
    };
} // namespace voxl
