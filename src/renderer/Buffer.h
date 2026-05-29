#pragma once
#include <vulkan/vulkan.h>

class Buffer
{
public:
    Buffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    ~Buffer();

    // Blokujemy kopiowanie
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    [[nodiscard]] VkBuffer getBuffer() const { return m_buffer; }
    void copyData(const void* data, VkDeviceSize size);

private:
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_bufferMemory = VK_NULL_HANDLE;
    VkDeviceSize m_size;
};
