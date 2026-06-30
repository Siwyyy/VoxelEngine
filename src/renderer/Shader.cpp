#include "Shader.h"

#include <stdexcept>

#include "utils/FileSystem.h"

namespace voxl
{
    Shader::Shader(VkDevice device, const std::string& filePath)
        : m_device(device)
    {
        const auto codeResult = FileSystem::readFile(filePath);
        if (!codeResult.has_value())
        {
            throw std::runtime_error(codeResult.error());
        }
        const auto& code = codeResult.value();

        const VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size(),
            .pCode = reinterpret_cast<const uint32_t*>(code.data())
        };

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
} // namespace voxl
