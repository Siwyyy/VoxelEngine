#pragma once
#include <array>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace voxl
{
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            return {
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            };
        }


        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
        {
            return std::array{
                VkVertexInputAttributeDescription{
                    .location = 0,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = static_cast<uint32_t>(offsetof(Vertex, pos))
                },
                VkVertexInputAttributeDescription{
                    .location = 1,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = static_cast<uint32_t>(offsetof(Vertex, color))
                }
            };
        }
    };
} // namespace voxl
