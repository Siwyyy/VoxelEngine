#pragma once
#include <unordered_map>
#include <tuple>
#include <vector>
#include <memory>
#include "Chunk.h"
#include "../utils/ThreadPool.h"
#include <future>
#include <glm/glm.hpp>
#include <vk_mem_alloc.h>

class World {
public:
    World(VkDevice device, VmaAllocator allocator);
    ~World();

    void update(const glm::vec3& cameraPos);
    
    const std::vector<Chunk*>& getActiveChunks() const { return m_activeChunks; }

private:
    VkDevice m_device;
    VmaAllocator m_allocator;

    struct ChunkCoord {
        int x, y, z;
        bool operator==(const ChunkCoord& o) const {
            return x == o.x && y == o.y && z == o.z;
        }
    };
    struct ChunkCoordHash {
        std::size_t operator()(const ChunkCoord& k) const {
            return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1) ^ (std::hash<int>()(k.z) << 2);
        }
    };
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> m_chunkMap;
    std::vector<Chunk*> m_activeChunks;
    
    std::unique_ptr<ThreadPool> m_threadPool;
    std::unordered_map<ChunkCoord, std::future<std::unique_ptr<Chunk>>, ChunkCoordHash> m_chunkFutures;
    int m_renderDistance = 8; 
};
