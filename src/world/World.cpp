#include "World.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <print>
#include <ranges>

#include "renderer/VulkanContext.h"

namespace voxl
{
    World::World(VulkanContext* context)
        : m_vulkanContext(context)
    {
        std::filesystem::create_directories(m_worldPath);
        const unsigned int hardwareThreads = std::thread::hardware_concurrency();
        int workerThreads                  = std::max(2, static_cast<int>(hardwareThreads) - 2);
        m_threadPool                       = std::make_unique<ThreadPool>(workerThreads);

        std::println("Initializing World Manager (render distance: {})...", m_renderDistance);
    }

    World::~World() noexcept // NOLINT(bugprone-exception-escape)
    {
        for (auto& future: m_chunkFutures | std::views::values)
        {
            future.wait();
        }
        try
        {
            saveAllChunks();
            m_chunkMap.clear();
        }
        catch (const std::exception& e)
        {
            std::println(stderr, "Exception while saving world in destructor: {}", e.what());
        }
        catch (...)
        {
            std::println(stderr, "Unknown exception while saving world in destructor.");
        }
    }

    float World::getEstimatedMemoryUsageGB() const
    {
        const size_t totalChunks = m_chunkMap.size() + m_chunkFutures.size() + m_chunksToDelete.size();
        return static_cast<float>(totalChunks * sizeof(Chunk)) / (1024.0f * 1024.0f * 1024.0f);
    }

    void World::update(const glm::vec3& cameraPos, const glm::vec3& cameraFront, float deltaTime)
    {
        m_updateTimer          += deltaTime;
        bool shouldCheckChunks = false;
        if (m_updateTimer >= 0.1f)
        {
            m_updateTimer     = 0.0f;
            shouldCheckChunks = true;
        }

        for (const auto& val: m_chunkMap | std::views::values)
        {
            if (val->isDirty())
            {
                vkDeviceWaitIdle(m_vulkanContext->getDevice());
                val->buildMesh();
                val->setDirty(false);
            }
        }

        constexpr float chunkPhysicalSize = Chunk::CHUNK_SIZE * 0.05f;
        const int currentChunkX           = static_cast<int>(std::floor(cameraPos.x / chunkPhysicalSize));
        const int currentChunkY           = static_cast<int>(std::floor(cameraPos.y / chunkPhysicalSize));
        const int currentChunkZ           = static_cast<int>(std::floor(cameraPos.z / chunkPhysicalSize));

        m_playerChunkX.store(currentChunkX);
        m_playerChunkY.store(currentChunkY);
        m_playerChunkZ.store(currentChunkZ);
        m_atomicRenderDistance.store(m_renderDistance);

        m_frameCount++;

        {
            std::scoped_lock lock(m_deletionMutex);
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

            auto coords = std::views::cartesian_product(std::views::iota(-m_renderDistance, m_renderDistance + 1),
                                                        std::views::iota(-m_renderDistance, m_renderDistance + 1),
                                                        std::views::iota(-m_renderDistance, m_renderDistance + 1));
            for (const auto [x, y, z]: coords)
            {
                const int r2 = (m_renderDistance + 2) * (m_renderDistance + 2);
                if (y * y + z * z <= r2 && x * x + y * y <= r2)
                {
                    ChunkCoord coord = {.x = currentChunkX + x, .y = currentChunkY + y, .z = currentChunkZ + z};
                    if (!m_chunkMap.contains(coord) && !m_chunkFutures.contains(coord))
                    {
                        glm::vec3 pos((static_cast<float>(coord.x) + 0.5f) * chunkPhysicalSize,
                                      (static_cast<float>(coord.y) + 0.5f) * chunkPhysicalSize,
                                      (static_cast<float>(coord.z) + 0.5f) * chunkPhysicalSize);
                        glm::vec3 dir     = pos - cameraPos;
                        const float dist  = glm::length(dir);
                        const float dot   = dist > 0.1f ? glm::dot(dir / dist, cameraFront) : 1.0f;
                        const float score = dist * (2.0f - dot);
                        expectedChunks.push_back({.coord = coord, .score = score});
                    }
                }
            }

            std::ranges::sort(expectedChunks, [](const ChunkScore& a, const ChunkScore& b)
            {
                return a.score < b.score;
            });

            for (const auto& [coord, score]: expectedChunks)
            {
                if (getEstimatedMemoryUsageGB() >= m_memoryLimitGB) break;

                glm::vec3 pos(static_cast<float>(coord.x) * Chunk::CHUNK_SIZE,
                              static_cast<float>(coord.y) * Chunk::CHUNK_SIZE,
                              static_cast<float>(coord.z) * Chunk::CHUNK_SIZE);
                std::string filepath = m_worldPath + "chunk_" + std::to_string(coord.x) + "_" +
                        std::to_string(coord.y) + "_" + std::to_string(coord.z) + ".bin";

                m_chunkFutures[coord] = m_threadPool->enqueue([this, pos, vb = m_vulkanContext->getMegaVertexBuffer(), ib = m_vulkanContext->
                        getMegaIndexBuffer(), filepath, coord]()
                    {
                        auto checkDistance = [this, &coord]()
                        {
                            const int px      = m_playerChunkX.load();
                            const int py      = m_playerChunkY.load();
                            const int pz      = m_playerChunkZ.load();
                            const int r       = m_atomicRenderDistance.load();
                            const int dx      = coord.x - px;
                            const int dy      = coord.y - py;
                            const int dz      = coord.z - pz;
                            const int r2      = (r + 2) * (r + 2);
                            const bool inCylX = (dy * dy + dz * dz <= r2);
                            const bool inCylZ = (dx * dx + dy * dy <= r2);
                            return !(inCylX && inCylZ);
                        };

                        if (checkDistance()) return std::unique_ptr<Chunk>(nullptr);

                        auto chunk = std::make_unique<Chunk>(pos, vb, ib);
                        if (!chunk->load(filepath))
                        {
                            chunk->generateTerrain();
                        }

                        if (checkDistance()) return std::unique_ptr<Chunk>(nullptr);

                        chunk->buildMesh();
                        return chunk;
                    });
            }

            for (auto it = m_chunkMap.begin(); it != m_chunkMap.end();)
            {
                const int dx      = it->first.x - currentChunkX;
                const int dy      = it->first.y - currentChunkY;
                const int dz      = it->first.z - currentChunkZ;
                const int r2      = (m_renderDistance + 2) * (m_renderDistance + 2);
                const bool inCylX = (dy * dy + dz * dz <= r2);
                const bool inCylZ = (dx * dx + dy * dy <= r2);
                if (!(inCylX && inCylZ))
                {
                    std::string filepath = std::format("{}chunk_{}_{}_{}.bin", m_worldPath, it->first.x, it->first.y, it->first.z);

                    auto chunk = std::move(it->second);
                    it         = m_chunkMap.erase(it);


                    m_threadPool->enqueue([this, chunk = std::move(chunk), filepath, currentFrame = m_frameCount]() mutable
                    {
                        chunk->save(filepath);

                        std::scoped_lock lock(m_deletionMutex);
                        m_chunksToDelete.push_back({.chunk = std::move(chunk), .frameRemoved = currentFrame});
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
                if (auto chunk = it->second.get())
                {
                    m_chunkMap[it->first] = std::move(chunk);
                    m_chunkMap[it->first]->setDirty(false);
                }
                it = m_chunkFutures.erase(it);
            }
            else
            {
                ++it;
            }
        }

        m_activeChunks.clear();
        for (auto& val: m_chunkMap | std::views::values)
        {
            if (val->getIndexCount() > 0 && val->hasValidAllocation())
            {
                m_activeChunks.push_back(val.get());
            }
        }

        // Sort front-to-back for Early-Z hardware occlusion culling
        std::ranges::sort(m_activeChunks, [&cameraPos](const Chunk* a, const Chunk* b)
        {
            const glm::vec3 centerA = (a->getPosition() + glm::vec3(Chunk::CHUNK_SIZE * 0.5f)) * 0.05f;
            const glm::vec3 centerB = (b->getPosition() + glm::vec3(Chunk::CHUNK_SIZE * 0.5f)) * 0.05f;

            const float distA = glm::length(centerA - cameraPos);
            const float distB = glm::length(centerB - cameraPos);

            return distA < distB;
        });
    }

    void World::processPlayerInteraction(const glm::vec3& cameraPos, const glm::vec3& cameraFront, const bool leftClick, const bool rightClick)
    {
        const glm::vec3 rayStart = cameraPos / 0.05f;
        const glm::vec3 rayDir   = glm::normalize(cameraFront);

        int x = static_cast<int>(std::floor(rayStart.x));
        int y = static_cast<int>(std::floor(rayStart.y));
        int z = static_cast<int>(std::floor(rayStart.z));

        const int stepX = (rayDir.x > 0) ? 1 : ((rayDir.x < 0) ? -1 : 0);
        const int stepY = (rayDir.y > 0) ? 1 : ((rayDir.y < 0) ? -1 : 0);
        const int stepZ = (rayDir.z > 0) ? 1 : ((rayDir.z < 0) ? -1 : 0);

        const float tDeltaX = (stepX != 0) ? std::abs(1.0f / rayDir.x) : std::numeric_limits<float>::max();
        const float tDeltaY = (stepY != 0) ? std::abs(1.0f / rayDir.y) : std::numeric_limits<float>::max();
        const float tDeltaZ = (stepZ != 0) ? std::abs(1.0f / rayDir.z) : std::numeric_limits<float>::max();

        const float nextBoundaryX = static_cast<float>(x) + (stepX > 0 ? 0.5f : -0.5f);
        const float nextBoundaryY = static_cast<float>(y) + (stepY > 0 ? 0.5f : -0.5f);
        const float nextBoundaryZ = static_cast<float>(z) + (stepZ > 0 ? 0.5f : -0.5f);

        float tMaxX = (stepX != 0) ? (nextBoundaryX - rayStart.x) / rayDir.x : std::numeric_limits<float>::max();
        float tMaxY = (stepY != 0) ? (nextBoundaryY - rayStart.y) / rayDir.y : std::numeric_limits<float>::max();
        float tMaxZ = (stepZ != 0) ? (nextBoundaryZ - rayStart.z) / rayDir.z : std::numeric_limits<float>::max();

        int lastFaceNormalX = 0;
        int lastFaceNormalY = 0;
        int lastFaceNormalZ = 0;

        for (int i = 0; i < 20; ++i)
        { // max reach 20 blocks
            const Block block = getBlockAt(x, y, z);
            if (block.type != BlockType::Air)
            {
                if (leftClick)
                {
                    setBlockAt(x, y, z, Block(BlockType::Air));
                }
                else if (rightClick)
                {
                    const int px = x - lastFaceNormalX;
                    const int py = y - lastFaceNormalY;
                    const int pz = z - lastFaceNormalZ;

                    const int camX = static_cast<int>(std::floor(rayStart.x + 0.5f));
                    const int camY = static_cast<int>(std::floor(rayStart.y + 0.5f));
                    const int camZ = static_cast<int>(std::floor(rayStart.z + 0.5f));

                    if (px != camX || py != camY || pz != camZ)
                    {
                        setBlockAt(px, py, pz, Block(BlockType::Stone));
                    }
                }
                break;
            }

            if (tMaxX < tMaxY)
            {
                if (tMaxX < tMaxZ)
                {
                    x               += stepX;
                    tMaxX           += tDeltaX;
                    lastFaceNormalX = stepX;
                    lastFaceNormalY = 0;
                    lastFaceNormalZ = 0;
                }
                else
                {
                    z               += stepZ;
                    tMaxZ           += tDeltaZ;
                    lastFaceNormalX = 0;
                    lastFaceNormalY = 0;
                    lastFaceNormalZ = stepZ;
                }
            }
            else
            {
                if (tMaxY < tMaxZ)
                {
                    y               += stepY;
                    tMaxY           += tDeltaY;
                    lastFaceNormalX = 0;
                    lastFaceNormalY = stepY;
                    lastFaceNormalZ = 0;
                }
                else
                {
                    z               += stepZ;
                    tMaxZ           += tDeltaZ;
                    lastFaceNormalX = 0;
                    lastFaceNormalY = 0;
                    lastFaceNormalZ = stepZ;
                }
            }
        }
    }

    void World::changeWorld(const std::string& newPath)
    {
        m_threadPool->clearTasks();
        m_threadPool.reset();

        vkDeviceWaitIdle(m_vulkanContext->getDevice());

        saveAllChunks();
        m_chunkMap.clear();
        m_chunkFutures.clear();
        m_activeChunks.clear();

        m_worldPath = newPath;
        std::filesystem::create_directories(m_worldPath);

        m_threadPool = std::make_unique<ThreadPool>(4);
    }

    void World::setBlockAt(int x, int y, int z, const Block block)
    {
        const auto chunkX = static_cast<int>(std::floor(static_cast<float>(x) / Chunk::CHUNK_SIZE));
        const auto chunkY = static_cast<int>(std::floor(static_cast<float>(y) / Chunk::CHUNK_SIZE));
        const auto chunkZ = static_cast<int>(std::floor(static_cast<float>(z) / Chunk::CHUNK_SIZE));

        const ChunkCoord coord = {.x = chunkX, .y = chunkY, .z = chunkZ};
        const auto it          = m_chunkMap.find(coord);
        if (it != m_chunkMap.end())
        {
            const int lx = x - chunkX * static_cast<int>(Chunk::CHUNK_SIZE);
            const int ly = y - chunkY * static_cast<int>(Chunk::CHUNK_SIZE);
            const int lz = z - chunkZ * static_cast<int>(Chunk::CHUNK_SIZE);
            if (it->second->setBlock(lx, ly, lz, block))
            {
                it->second->setDirty(true);

                if (lx == 0)
                {
                    const auto nIt = m_chunkMap.find({.x = chunkX - 1, .y = chunkY, .z = chunkZ});
                    if (nIt != m_chunkMap.end()) nIt->second->setDirty(true);
                }
                else if (lx == Chunk::CHUNK_SIZE - 1)
                {
                    const auto nIt = m_chunkMap.find({.x = chunkX + 1, .y = chunkY, .z = chunkZ});
                    if (nIt != m_chunkMap.end()) nIt->second->setDirty(true);
                }
                if (ly == 0)
                {
                    const auto nIt = m_chunkMap.find({.x = chunkX, .y = chunkY - 1, .z = chunkZ});
                    if (nIt != m_chunkMap.end()) nIt->second->setDirty(true);
                }
                else if (ly == Chunk::CHUNK_SIZE - 1)
                {
                    const auto nIt = m_chunkMap.find({.x = chunkX, .y = chunkY + 1, .z = chunkZ});
                    if (nIt != m_chunkMap.end()) nIt->second->setDirty(true);
                }
                if (lz == 0)
                {
                    const auto nIt = m_chunkMap.find({.x = chunkX, .y = chunkY, .z = chunkZ - 1});
                    if (nIt != m_chunkMap.end()) nIt->second->setDirty(true);
                }
                else if (lz == Chunk::CHUNK_SIZE - 1)
                {
                    const auto nIt = m_chunkMap.find({.x = chunkX, .y = chunkY, .z = chunkZ + 1});
                    if (nIt != m_chunkMap.end()) nIt->second->setDirty(true);
                }
            }
        }
    }

    Block World::getBlockAt(int x, int y, int z) const
    {
        const int chunkX = static_cast<int>(std::floor(static_cast<float>(x) / Chunk::CHUNK_SIZE));
        const int chunkY = static_cast<int>(std::floor(static_cast<float>(y) / Chunk::CHUNK_SIZE));
        const int chunkZ = static_cast<int>(std::floor(static_cast<float>(z) / Chunk::CHUNK_SIZE));

        const ChunkCoord coord = {.x = chunkX, .y = chunkY, .z = chunkZ};
        const auto it          = m_chunkMap.find(coord);
        if (it != m_chunkMap.end())
        {
            const int lx = x - chunkX * static_cast<int>(Chunk::CHUNK_SIZE);
            const int ly = y - chunkY * static_cast<int>(Chunk::CHUNK_SIZE);
            const int lz = z - chunkZ * static_cast<int>(Chunk::CHUNK_SIZE);
            return it->second->getBlock(lx, ly, lz);
        }
        return {BlockType::Air};
    }

    void World::saveAllChunks() const
    {
        for (const auto& [fst, snd]: m_chunkMap)
        {
            std::string filepath = std::format("{}chunk_{}_{}_{}.bin", m_worldPath, fst.x, fst.y, fst.z);
            snd->save(filepath);
        }
    }
} // namespace voxl
