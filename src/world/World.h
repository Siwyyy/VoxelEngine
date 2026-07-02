#pragma once
#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "Chunk.h"
#include "utils/ThreadPool.h"

namespace voxl
{
    class VulkanContext;

    class World
    {
    public:
        World() = delete;
        explicit World(VulkanContext* context);
        World(const World&)     = delete;
        World(World&&) noexcept = delete;
        ~World() noexcept;
        World& operator=(const World&)     = delete;
        World& operator=(World&&) noexcept = delete;

        void update(const glm::vec3& cameraPos, const glm::vec3& cameraFront, float deltaTime);
        void processPlayerInteraction(const glm::vec3& cameraPos, const glm::vec3& cameraFront, bool leftClick, bool rightClick);

        void changeWorld(const std::string& newPath);
        [[nodiscard]] const std::string& getWorldPath() const { return m_worldPath; }

        void setRenderDistance(int distance) { m_renderDistance = distance; }
        [[nodiscard]] int getRenderDistance() const { return m_renderDistance; }

        void setBlockAt(int x, int y, int z, Block block);
        [[nodiscard]] Block getBlockAt(int x, int y, int z) const;

        void saveAllChunks() const;
        [[nodiscard]] const std::vector<Chunk*>& getActiveChunks() const { return m_activeChunks; }
        
        [[nodiscard]] size_t getQueuedChunksCount() const { return m_chunkFutures.size(); }
        [[nodiscard]] size_t getChunksToDeleteCount() const { return m_chunksToDelete.size(); }

        void setMemoryLimitGB(float limit) { m_memoryLimitGB = limit; }
        [[nodiscard]] float getMemoryLimitGB() const { return m_memoryLimitGB; }
        [[nodiscard]] float getEstimatedMemoryUsageGB() const;

    private:
        VulkanContext* m_vulkanContext;

        struct ChunkCoord
        {
            int x, y, z;

            bool operator==(const ChunkCoord& other) const { return x == other.x && y == other.y && z == other.z; }
        };

        struct ChunkCoordHash
        {
            std::size_t operator()(const ChunkCoord& k) const
            {
                return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1) ^ (std::hash<int>()(k.z) << 2);
            }
        };

        std::atomic<int> m_playerChunkX{0};
        std::atomic<int> m_playerChunkY{0};
        std::atomic<int> m_playerChunkZ{0};
        std::atomic<int> m_atomicRenderDistance{0};

        std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> m_chunkMap;

        std::vector<Chunk*> m_activeChunks;

        int m_renderDistance    = 8;
        float m_memoryLimitGB   = 8.0f;
        float m_updateTimer     = 0.0f;
        std::string m_worldPath = "saves/world1/";

        struct ChunkGarbage
        {
            std::unique_ptr<Chunk> chunk;
            uint64_t frameRemoved;
        };

        std::vector<ChunkGarbage> m_chunksToDelete;
        std::mutex m_deletionMutex;
        uint64_t m_frameCount = 0;

        std::unordered_map<ChunkCoord, std::future<std::unique_ptr<Chunk>>, ChunkCoordHash> m_chunkFutures;
        std::unique_ptr<ThreadPool> m_threadPool;
    };
} // namespace voxl
