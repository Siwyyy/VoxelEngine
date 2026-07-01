#pragma once
#include <list>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace voxl
{
    struct BlockAllocation
    {
        uint32_t offset = 0;
        uint32_t size   = 0;
    };

    class MegaBuffer
    {
    public:
        MegaBuffer(VmaAllocator allocator, VkDeviceSize capacity, VkBufferUsageFlags usage);
        ~MegaBuffer();

        std::optional<BlockAllocation> allocate(uint32_t size);
        void free(const BlockAllocation& block);

        template<typename T>
        void upload(const BlockAllocation& block, std::span<const T> data) const
        {
            if (data.empty()) return;
            const size_t bytesToCopy = data.size_bytes();
            if (bytesToCopy > block.size) throw std::runtime_error("Upload data exceeds block size!");
            std::memcpy(static_cast<char*>(m_mappedData) + block.offset, data.data(), bytesToCopy);
        }

        [[nodiscard]] VkBuffer getBuffer() const { return m_buffer; }

    private:
        VmaAllocator m_allocator;
        VkBuffer m_buffer          = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;
        void* m_mappedData         = nullptr;

        std::list<BlockAllocation> m_freeBlocks;
        std::mutex m_mutex;
    };
} // namespace voxl
