#pragma once
#include <unordered_map>
#include <tuple>
#include <vector>
#include <memory>
#include <string>
#include <filesystem>
#include "Chunk.h"
#include "../utils/ThreadPool.h"
#include <future>
#include <glm/glm.hpp>

class VulkanContext;

class World
{
public:
    World(VulkanContext* context);
    ~World();

    void update(const glm::vec3& cameraPos, const glm::vec3& cameraFront, float deltaTime);
    void changeWorld(const std::string& newPath);
    const std::string& getWorldPath() const { return m_worldPath; }

    int getRenderDistance() const { return m_renderDistance; }
    void setRenderDistance(int distance) { m_renderDistance = distance; }

    const std::vector<Chunk*>& getActiveChunks() const { return m_activeChunks; }

private:
    VulkanContext* m_vulkanContext;

    struct ChunkCoord
    {
        int x, y, z;

        bool operator==(const ChunkCoord& other) const
        {
            return x == other.x && y == other.y && z == other.z;
        }
    };

    struct ChunkCoordHash
    {
        std::size_t operator()(const ChunkCoord& k) const
        {
            return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1) ^ (std::hash<int>()(k.z) << 2);
        }
    };

    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> m_chunkMap;
    std::vector<Chunk*> m_activeChunks;

    std::unique_ptr<ThreadPool> m_threadPool;
    std::unordered_map<ChunkCoord, std::future<std::unique_ptr<Chunk>>, ChunkCoordHash> m_chunkFutures;

    std::vector<ChunkCoord> m_spiralPattern;

    int m_renderDistance = 8;
    float m_updateTimer = 0.0f;
    std::string m_worldPath = "saves/world1/";

    struct ChunkGarbage
    {
        std::unique_ptr<Chunk> chunk;
        uint64_t frameRemoved;
    };

    std::vector<ChunkGarbage> m_chunksToDelete;
    std::mutex m_deletionMutex;
    uint64_t m_frameCount = 0;

    void generateSpiralPattern();
};
