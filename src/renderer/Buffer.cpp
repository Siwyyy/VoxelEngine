#include "Buffer.h"

#include <cstring>
#include <stdexcept>

namespace voxl
{
    Buffer::Buffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
        : m_allocator(allocator)
    {
        const VkBufferCreateInfo bufferInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        const VmaAllocationCreateInfo allocInfo{
            .usage = memoryUsage
        };

        if (vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &m_buffer, &m_allocation, nullptr) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create buffer!");
        }
    }

    Buffer::~Buffer()
    {
        if (m_buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
        }
    }

    void Buffer::copyData(const void* data, const VkDeviceSize size) const
    {
        void* mappedData;
        vmaMapMemory(m_allocator, m_allocation, &mappedData);
        memcpy(mappedData, data, static_cast<size_t>(size));
        vmaUnmapMemory(m_allocator, m_allocation);
    }
} // namespace voxl
