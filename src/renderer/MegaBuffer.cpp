#include "MegaBuffer.h"

#include <algorithm>
#include <stdexcept>

namespace voxl
{
    MegaBuffer::MegaBuffer(VkDevice device, VmaAllocator allocator, VkDeviceSize capacity, VkBufferUsageFlags usage)
        : m_allocator(allocator)
    {
        const VkBufferCreateInfo bufferInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = capacity,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        const VmaAllocationCreateInfo allocInfo{
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

    BlockAllocation MegaBuffer::allocate(uint32_t size)
    {
        if (size == 0) return {.offset = 0, .size = 0, .valid = false};
        std::scoped_lock lock(m_mutex);
        for (auto it = m_freeBlocks.begin(); it != m_freeBlocks.end(); ++it)
        {
            if (it->size >= size)
            {
                BlockAllocation alloc;
                alloc.offset = it->offset;
                alloc.size   = size;
                alloc.valid  = true;

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
        return {.offset = 0, .size = 0, .valid = false}; // Out of memory
    }

    void MegaBuffer::free(const BlockAllocation& block)
    {
        if (!block.valid) return;
        std::scoped_lock lock(m_mutex);

        m_freeBlocks.push_back({.offset = block.offset, .size = block.size});
        m_freeBlocks.sort([](const FreeBlock& a, const FreeBlock& b)
        {
            return a.offset < b.offset;
        });

        for (auto it = m_freeBlocks.begin(); it != m_freeBlocks.end();)
        {
            if (auto next = std::next(it); next != m_freeBlocks.end() && it->offset + it->size == next->offset)
            {
                it->size += next->size;
                m_freeBlocks.erase(next);
            }
            else
            {
                ++it;
            }
        }
    }

    void MegaBuffer::upload(const BlockAllocation& block, const void* data) const
    {
        if (!block.valid || !data) return;
        std::memcpy(static_cast<char*>(m_mappedData) + block.offset, data, block.size);
    }
} // namespace voxl
