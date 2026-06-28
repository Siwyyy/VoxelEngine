#pragma once
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace voxl
{
    class Buffer
    {
    public:
        Buffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage,
               VmaMemoryUsage memoryUsage);
        ~Buffer();

        Buffer(const Buffer&)            = delete;
        Buffer& operator=(const Buffer&) = delete;

        [[nodiscard]] VkBuffer getBuffer() const { return m_buffer; }
        void copyData(const void* data, VkDeviceSize size) const;

    private:
        VmaAllocator m_allocator;
        VkBuffer m_buffer          = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;
    };
} // namespace voxl
