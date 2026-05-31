#include "World.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <sstream>
#include "../renderer/VulkanContext.h"

World::World(VulkanContext* context)
    : m_vulkanContext(context)
{
    std::filesystem::create_directories(m_worldPath);
    unsigned int hardwareThreads = std::thread::hardware_concurrency();
    int workerThreads = std::max(2, static_cast<int>(hardwareThreads) - 2);
    m_threadPool = std::make_unique<ThreadPool>(workerThreads);

    std::cout << "Inicjalizacja Managera Swiata (render distance: " << m_renderDistance << ")...\n";
}

World::~World()
{
    for (auto& pair : m_chunkMap)
    {
        std::stringstream ss;
        ss << m_worldPath << "chunk_" << pair.first.x << "_" << pair.first.y << "_" << pair.first.z << ".bin";
        pair.second->save(ss.str());
    }
    m_chunkMap.clear();
}

void World::saveAllChunks()
{
    for (auto& pair : m_chunkMap)
    {
        std::stringstream ss;
        ss << m_worldPath << "chunk_" << pair.first.x << "_" << pair.first.y << "_" << pair.first.z << ".bin";
        pair.second->save(ss.str());
    }
}

void World::changeWorld(const std::string& newPath)
{
    m_threadPool->clearTasks();
    m_threadPool.reset();

    vkDeviceWaitIdle(m_vulkanContext->getDevice());

    for (auto& pair : m_chunkMap)
    {
        std::stringstream ss;
        ss << m_worldPath << "chunk_" << pair.first.x << "_" << pair.first.y << "_" << pair.first.z << ".bin";
        pair.second->save(ss.str());
    }
    m_chunkMap.clear();
    m_chunkFutures.clear();
    m_activeChunks.clear();

    m_worldPath = newPath;
    std::filesystem::create_directories(m_worldPath);

    m_threadPool = std::make_unique<ThreadPool>(4);
}

Block World::getBlockAt(int x, int y, int z) const {
    int chunkX = static_cast<int>(std::floor(static_cast<float>(x) / Chunk::CHUNK_SIZE));
    int chunkY = static_cast<int>(std::floor(static_cast<float>(y) / Chunk::CHUNK_SIZE));
    int chunkZ = static_cast<int>(std::floor(static_cast<float>(z) / Chunk::CHUNK_SIZE));

    ChunkCoord coord = {chunkX, chunkY, chunkZ};
    auto it = m_chunkMap.find(coord);
    if (it != m_chunkMap.end()) {
        int lx = x - chunkX * Chunk::CHUNK_SIZE;
        int ly = y - chunkY * Chunk::CHUNK_SIZE;
        int lz = z - chunkZ * Chunk::CHUNK_SIZE;
        return it->second->getBlock(lx, ly, lz);
    }
    return Block(BlockType::Air);
}

void World::setBlockAt(int x, int y, int z, Block block) {
    int chunkX = static_cast<int>(std::floor(static_cast<float>(x) / Chunk::CHUNK_SIZE));
    int chunkY = static_cast<int>(std::floor(static_cast<float>(y) / Chunk::CHUNK_SIZE));
    int chunkZ = static_cast<int>(std::floor(static_cast<float>(z) / Chunk::CHUNK_SIZE));

    ChunkCoord coord = {chunkX, chunkY, chunkZ};
    auto it = m_chunkMap.find(coord);
    if (it != m_chunkMap.end()) {
        int lx = x - chunkX * Chunk::CHUNK_SIZE;
        int ly = y - chunkY * Chunk::CHUNK_SIZE;
        int lz = z - chunkZ * Chunk::CHUNK_SIZE;
        if (it->second->setBlock(lx, ly, lz, block)) {
            it->second->setDirty(true);
            
            if (lx == 0) {
                auto nIt = m_chunkMap.find({chunkX - 1, chunkY, chunkZ});
                if (nIt != m_chunkMap.end()) nIt->second->setDirty(true);
            } else if (lx == Chunk::CHUNK_SIZE - 1) {
                auto nIt = m_chunkMap.find({chunkX + 1, chunkY, chunkZ});
                if (nIt != m_chunkMap.end()) nIt->second->setDirty(true);
            }
            if (ly == 0) {
                auto nIt = m_chunkMap.find({chunkX, chunkY - 1, chunkZ});
                if (nIt != m_chunkMap.end()) nIt->second->setDirty(true);
            } else if (ly == Chunk::CHUNK_SIZE - 1) {
                auto nIt = m_chunkMap.find({chunkX, chunkY + 1, chunkZ});
                if (nIt != m_chunkMap.end()) nIt->second->setDirty(true);
            }
            if (lz == 0) {
                auto nIt = m_chunkMap.find({chunkX, chunkY, chunkZ - 1});
                if (nIt != m_chunkMap.end()) nIt->second->setDirty(true);
            } else if (lz == Chunk::CHUNK_SIZE - 1) {
                auto nIt = m_chunkMap.find({chunkX, chunkY, chunkZ + 1});
                if (nIt != m_chunkMap.end()) nIt->second->setDirty(true);
            }
        }
    }
}

void World::processPlayerInteraction(const glm::vec3& cameraPos, const glm::vec3& cameraFront, bool leftClick, bool rightClick)
{
    glm::vec3 rayStart = cameraPos / 0.05f;
    glm::vec3 rayDir = glm::normalize(cameraFront);

    int x = static_cast<int>(std::floor(rayStart.x));
    int y = static_cast<int>(std::floor(rayStart.y));
    int z = static_cast<int>(std::floor(rayStart.z));

    int stepX = (rayDir.x > 0) ? 1 : ((rayDir.x < 0) ? -1 : 0);
    int stepY = (rayDir.y > 0) ? 1 : ((rayDir.y < 0) ? -1 : 0);
    int stepZ = (rayDir.z > 0) ? 1 : ((rayDir.z < 0) ? -1 : 0);

    float tDeltaX = (stepX != 0) ? std::abs(1.0f / rayDir.x) : std::numeric_limits<float>::max();
    float tDeltaY = (stepY != 0) ? std::abs(1.0f / rayDir.y) : std::numeric_limits<float>::max();
    float tDeltaZ = (stepZ != 0) ? std::abs(1.0f / rayDir.z) : std::numeric_limits<float>::max();

    float nextBoundaryX = static_cast<float>(x) + (stepX > 0 ? 0.5f : -0.5f);
    float nextBoundaryY = static_cast<float>(y) + (stepY > 0 ? 0.5f : -0.5f);
    float nextBoundaryZ = static_cast<float>(z) + (stepZ > 0 ? 0.5f : -0.5f);

    float tMaxX = (stepX != 0) ? (nextBoundaryX - rayStart.x) / rayDir.x : std::numeric_limits<float>::max();
    float tMaxY = (stepY != 0) ? (nextBoundaryY - rayStart.y) / rayDir.y : std::numeric_limits<float>::max();
    float tMaxZ = (stepZ != 0) ? (nextBoundaryZ - rayStart.z) / rayDir.z : std::numeric_limits<float>::max();

    int lastFaceNormalX = 0;
    int lastFaceNormalY = 0;
    int lastFaceNormalZ = 0;

    for (int i = 0; i < 20; ++i) { // max reach 20 blocks
        Block block = getBlockAt(x, y, z);
        if (block.type != BlockType::Air) {
            if (leftClick) {
                setBlockAt(x, y, z, Block(BlockType::Air));
            } else if (rightClick) {
                int px = x - lastFaceNormalX;
                int py = y - lastFaceNormalY;
                int pz = z - lastFaceNormalZ;
                
                int camX = static_cast<int>(std::floor(rayStart.x + 0.5f));
                int camY = static_cast<int>(std::floor(rayStart.y + 0.5f));
                int camZ = static_cast<int>(std::floor(rayStart.z + 0.5f));
                
                if (px != camX || py != camY || pz != camZ) {
                    setBlockAt(px, py, pz, Block(BlockType::Wood));
                }
            }
            break;
        }

        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                x += stepX;
                tMaxX += tDeltaX;
                lastFaceNormalX = stepX;
                lastFaceNormalY = 0;
                lastFaceNormalZ = 0;
            } else {
                z += stepZ;
                tMaxZ += tDeltaZ;
                lastFaceNormalX = 0;
                lastFaceNormalY = 0;
                lastFaceNormalZ = stepZ;
            }
        } else {
            if (tMaxY < tMaxZ) {
                y += stepY;
                tMaxY += tDeltaY;
                lastFaceNormalX = 0;
                lastFaceNormalY = stepY;
                lastFaceNormalZ = 0;
            } else {
                z += stepZ;
                tMaxZ += tDeltaZ;
                lastFaceNormalX = 0;
                lastFaceNormalY = 0;
                lastFaceNormalZ = stepZ;
            }
        }
    }
}

void World::update(const glm::vec3& cameraPos, const glm::vec3& cameraFront, float deltaTime)
{
    m_updateTimer += deltaTime;
    bool shouldCheckChunks = false;
    if (m_updateTimer >= 0.1f)
    {
        m_updateTimer = 0.0f;
        shouldCheckChunks = true;
    }

    for (auto& pair : m_chunkMap) {
        if (pair.second->isDirty()) {
            vkDeviceWaitIdle(m_vulkanContext->getDevice());
            pair.second->buildMesh();
            pair.second->setDirty(false);
        }
    }

    float chunkPhysicalSize = Chunk::CHUNK_SIZE * 0.05f;
    int currentChunkX = static_cast<int>(std::floor(cameraPos.x / chunkPhysicalSize));
    int currentChunkY = static_cast<int>(std::floor(cameraPos.y / chunkPhysicalSize));
    int currentChunkZ = static_cast<int>(std::floor(cameraPos.z / chunkPhysicalSize));

    m_frameCount++;

    {
        std::lock_guard<std::mutex> lock(m_deletionMutex);
        for (auto it = m_chunksToDelete.begin(); it != m_chunksToDelete.end();)
        {
            if (m_frameCount - it->frameRemoved > 3)
            {
                it = m_chunksToDelete.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    if (shouldCheckChunks)
    {
        struct ChunkScore
        {
            ChunkCoord coord;
            float score;
        };
        std::vector<ChunkScore> expectedChunks;

        for (int x = -m_renderDistance; x <= m_renderDistance; ++x)
        {
            for (int y = -m_renderDistance; y <= m_renderDistance; ++y)
            {
                for (int z = -m_renderDistance; z <= m_renderDistance; ++z)
                {
                    if (x * x + y * y + z * z <= m_renderDistance * m_renderDistance)
                    {
                        ChunkCoord coord = {currentChunkX + x, currentChunkY + y, currentChunkZ + z};
                        if (m_chunkMap.find(coord) == m_chunkMap.end() && m_chunkFutures.find(coord) == m_chunkFutures.
                            end())
                        {
                            glm::vec3 pos((coord.x + 0.5f) * chunkPhysicalSize, (coord.y + 0.5f) * chunkPhysicalSize,
                                          (coord.z + 0.5f) * chunkPhysicalSize);
                            glm::vec3 dir = pos - cameraPos;
                            float dist = glm::length(dir);
                            float dot = dist > 0.1f ? glm::dot(dir / dist, cameraFront) : 1.0f;
                            float score = dist * (2.0f - dot);
                            expectedChunks.push_back({coord, score});
                        }
                    }
                }
            }
        }

        std::sort(expectedChunks.begin(), expectedChunks.end(), [](const ChunkScore& a, const ChunkScore& b)
        {
            return a.score < b.score;
        });

        for (const auto& item : expectedChunks)
        {
            glm::vec3 pos(item.coord.x * Chunk::CHUNK_SIZE, item.coord.y * Chunk::CHUNK_SIZE,
                          item.coord.z * Chunk::CHUNK_SIZE);
            std::string filepath = m_worldPath + "chunk_" + std::to_string(item.coord.x) + "_" +
                std::to_string(item.coord.y) + "_" + std::to_string(item.coord.z) + ".bin";

            m_chunkFutures[item.coord] = m_threadPool->enqueue(
                [pos, vb = m_vulkanContext->getMegaVertexBuffer(), ib = m_vulkanContext->getMegaIndexBuffer(), filepath
                ]()
                {
                    auto chunk = std::make_unique<Chunk>(pos, vb, ib);
                    if (!chunk->load(filepath))
                    {
                        chunk->generateTerrain();
                    }
                    chunk->buildMesh();
                    return chunk;
                });
        }

        for (auto it = m_chunkMap.begin(); it != m_chunkMap.end();)
        {
            int dx = it->first.x - currentChunkX;
            int dy = it->first.y - currentChunkY;
            int dz = it->first.z - currentChunkZ;
            if (dx * dx + dy * dy + dz * dz > (m_renderDistance + 2) * (m_renderDistance + 2))
            {
                std::stringstream ss;
                ss << m_worldPath << "chunk_" << it->first.x << "_" << it->first.y << "_" << it->first.z << ".bin";
                std::string filepath = ss.str();

                auto chunk = std::move(it->second);
                it = m_chunkMap.erase(it);

                uint64_t currentFrame = m_frameCount;
                m_threadPool->enqueue([this, chunk = std::move(chunk), filepath, currentFrame]() mutable
                {
                    chunk->save(filepath);

                    std::lock_guard<std::mutex> lock(m_deletionMutex);
                    m_chunksToDelete.push_back({std::move(chunk), currentFrame});
                });
            }
            else
            {
                ++it;
            }
        }
    }

    for (auto it = m_chunkFutures.begin(); it != m_chunkFutures.end();)
    {
        if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            m_chunkMap[it->first] = it->second.get();
            it = m_chunkFutures.erase(it);
        }
        else
        {
            ++it;
        }
    }

    m_activeChunks.clear();
    for (auto& pair : m_chunkMap)
    {
        if (pair.second->getIndexCount() > 0 && pair.second->hasValidAllocation())
        {
            m_activeChunks.push_back(pair.second.get());
        }
    }

    // Sort front-to-back for Early-Z hardware occlusion culling
    std::sort(m_activeChunks.begin(), m_activeChunks.end(), [&cameraPos](Chunk* a, Chunk* b)
    {
        glm::vec3 centerA = (a->getPosition() + glm::vec3(Chunk::CHUNK_SIZE * 0.5f)) * 0.05f;
        glm::vec3 centerB = (b->getPosition() + glm::vec3(Chunk::CHUNK_SIZE * 0.5f)) * 0.05f;

        float distA = glm::length(centerA - cameraPos);
        float distB = glm::length(centerB - cameraPos);

        return distA < distB;
    });
}
