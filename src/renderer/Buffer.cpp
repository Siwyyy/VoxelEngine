#include "Buffer.h"
#include <stdexcept>
#include <cstring>

Buffer::Buffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
    : m_allocator(allocator), m_size(size)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;

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

void Buffer::copyData(const void* data, VkDeviceSize size)
{
    void* mappedData;
    vmaMapMemory(m_allocator, m_allocation, &mappedData);
    memcpy(mappedData, data, (size_t)size);
    vmaUnmapMemory(m_allocator, m_allocation);
}
