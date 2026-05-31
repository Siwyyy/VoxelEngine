#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

class Buffer
{
public:
    Buffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage,
           VmaMemoryUsage memoryUsage);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    [[nodiscard]] VkBuffer getBuffer() const { return m_buffer; }
    void copyData(const void* data, VkDeviceSize size);

private:
    VmaAllocator m_allocator;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
};
