#include "World.h"
#include <iostream>
#include <cmath>
#include <algorithm>

World::World(VkDevice device, VmaAllocator allocator)
    : m_device(device), m_allocator(allocator)
{
    m_threadPool = std::make_unique<ThreadPool>(4); // 4 threads for generation

    std::cout << "Inicjalizacja Managera Swiata (render distance: " << m_renderDistance << ")...\n";
}

World::~World()
{
    m_chunkMap.clear();
}

void World::update(const glm::vec3& cameraPos)
{
    float chunkPhysicalSize = Chunk::CHUNK_SIZE * 0.05f;
    int currentChunkX = static_cast<int>(std::floor(cameraPos.x / chunkPhysicalSize));
    int currentChunkY = static_cast<int>(std::floor(cameraPos.y / chunkPhysicalSize));
    int currentChunkZ = static_cast<int>(std::floor(cameraPos.z / chunkPhysicalSize));

    std::vector<ChunkCoord> expectedChunks;
    for (int x = -m_renderDistance; x <= m_renderDistance; ++x) {
        for (int y = -m_renderDistance; y <= m_renderDistance; ++y) {
            for (int z = -m_renderDistance; z <= m_renderDistance; ++z) {
                if (x*x + y*y + z*z <= m_renderDistance * m_renderDistance) {
                    expectedChunks.push_back({currentChunkX + x, currentChunkY + y, currentChunkZ + z});
                }
            }
        }
    }

    for (auto it = m_chunkMap.begin(); it != m_chunkMap.end(); ) {
        int dx = it->first.x - currentChunkX;
        int dy = it->first.y - currentChunkY;
        int dz = it->first.z - currentChunkZ;
        if (dx*dx + dy*dy + dz*dz > (m_renderDistance + 2) * (m_renderDistance + 2)) {
            vkDeviceWaitIdle(m_device);
            it = m_chunkMap.erase(it);
        } else {
            ++it;
        }
    }

    
    for (const auto& coord : expectedChunks) {
        if (m_chunkMap.find(coord) == m_chunkMap.end() && m_chunkFutures.find(coord) == m_chunkFutures.end()) {
            glm::vec3 pos(coord.x * Chunk::CHUNK_SIZE, coord.y * Chunk::CHUNK_SIZE, coord.z * Chunk::CHUNK_SIZE);
            m_chunkFutures[coord] = m_threadPool->enqueue([pos, allocator = m_allocator]() {
                auto chunk = std::make_unique<Chunk>(pos, allocator);
                chunk->generateTerrain();
                chunk->buildMesh();
                return chunk;
            });
        }
    }

    for (auto it = m_chunkFutures.begin(); it != m_chunkFutures.end(); ) {
        if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            m_chunkMap[it->first] = it->second.get();
            it = m_chunkFutures.erase(it);
        } else {
            ++it;
        }
    }

    m_activeChunks.clear();
    for (const auto& pair : m_chunkMap) {
        if (pair.second->getIndexCount() > 0) { // Only render chunks that actually have meshes
            m_activeChunks.push_back(pair.second.get());
        }
    }
}
