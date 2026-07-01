#include "MegaBuffer.h"

#include <algorithm>
#include <stdexcept>

namespace voxl
{
    MegaBuffer::MegaBuffer(VmaAllocator allocator, VkDeviceSize capacity, VkBufferUsageFlags usage)
        : m_allocator(allocator)
    {
        const VkBufferCreateInfo bufferInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = capacity,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        constexpr VmaAllocationCreateInfo allocInfo{
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
        };

        VmaAllocationInfo vmaAllocInfo;
        if (vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &m_buffer, &m_allocation, &vmaAllocInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create MegaBuffer!");
        }

        m_mappedData = vmaAllocInfo.pMappedData;
        m_freeBlocks.push_back({.offset = 0, .size = static_cast<uint32_t>(capacity)});
    }

    MegaBuffer::~MegaBuffer()
    {
        if (m_buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
        }
    }

    std::optional<BlockAllocation> MegaBuffer::allocate(uint32_t size)
    {
        if (size == 0) return std::nullopt;
        std::scoped_lock lock(m_mutex);
        for (auto it = m_freeBlocks.begin(); it != m_freeBlocks.end(); ++it)
        {
            if (it->size >= size)
            {
                BlockAllocation alloc;
                alloc.offset = it->offset;
                alloc.size   = size;

                if (it->size == size)
                {
                    m_freeBlocks.erase(it);
                }
                else
                {
                    it->offset += size;
                    it->size   -= size;
                }
                return alloc;
            }
        }
        return std::nullopt; // Out of memory
    }

    void MegaBuffer::free(const BlockAllocation& block)
    {
        std::scoped_lock lock(m_mutex);

        const auto it = m_freeBlocks.insert(std::ranges::find_if(m_freeBlocks, [&](const BlockAllocation& fb)
        {
            return fb.offset > block.offset;
        }), block);

        if (const auto next = std::next(it); next != m_freeBlocks.end() && it->offset + it->size == next->offset)
        {
            it->size += next->size;
            m_freeBlocks.erase(next);
        }

        if (it != m_freeBlocks.begin())
        {
            if (const auto prev = std::prev(it); prev->offset + prev->size == it->offset)
            {
                prev->size += it->size;
                m_freeBlocks.erase(it);
            }
        }
    }
} // namespace voxl
