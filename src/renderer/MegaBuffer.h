#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <list>
#include <mutex>

struct BlockAllocation
{
    uint32_t offset;
    uint32_t size;
    bool valid = false;
};

class MegaBuffer
{
public:
    MegaBuffer(VkDevice device, VmaAllocator allocator, VkDeviceSize capacity, VkBufferUsageFlags usage);
    ~MegaBuffer();

    BlockAllocation allocate(uint32_t size);
    void free(const BlockAllocation& block);

    void upload(const BlockAllocation& block, const void* data) const;

    [[nodiscard]] VkBuffer getBuffer() const { return m_buffer; }

private:
    VmaAllocator m_allocator;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    void* m_mappedData = nullptr;

    struct FreeBlock
    {
        uint32_t offset;
        uint32_t size;
    };

    std::list<FreeBlock> m_freeBlocks;
    std::mutex m_mutex;
};
