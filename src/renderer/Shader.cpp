#include "Shader.h"

#include <stdexcept>

#include "utils/FileSystem.h"

Shader::Shader(VkDevice device, const std::string& filePath)
    : m_device(device)
{
    const auto code = FileSystem::readFile(filePath);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &m_module) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module: " + filePath);
    }
}

Shader::~Shader()
{
    if (m_module != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_device, m_module, nullptr);
    }
}

Shader::Shader(Shader&& other) noexcept
    : m_device(other.m_device), m_module(other.m_module)
{
    other.m_module = VK_NULL_HANDLE;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other)
    {
        if (m_module != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_device, m_module, nullptr);
        }
        m_device       = other.m_device;
        m_module       = other.m_module;
        other.m_module = VK_NULL_HANDLE;
    }
    return *this;
}
