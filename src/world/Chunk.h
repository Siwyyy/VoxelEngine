#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "../renderer/Buffer.h"
#include "../renderer/Vertex.h"
#include "../utils/FastNoiseLite.h"

enum class BlockType : uint8_t {
    Air = 0,
    Grass = 1,
    Dirt = 2,
    Stone = 3,
    Wood = 4,
    Leaves = 5
};

class Chunk {
public:
    static const int CHUNK_SIZE = 64;

    Chunk(glm::vec3 position, VmaAllocator allocator);
    ~Chunk();

    void generateTerrain();
    void buildMesh();
    void draw(VkCommandBuffer commandBuffer);

    void setBlock(int x, int y, int z, BlockType type);
    BlockType getBlock(int x, int y, int z) const;
    
    [[nodiscard]] uint32_t getIndexCount() const { return m_indexCount; }
    [[nodiscard]] glm::vec3 getPosition() const { return m_position; }


private:
    bool isFaceVisible(int x, int y, int z) const;
    void addFace(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int x, int y, int z, int faceIndex, BlockType type);

    glm::vec3 m_position;
    VmaAllocator m_allocator;

    std::unique_ptr<Buffer> m_vertexBuffer;
    std::unique_ptr<Buffer> m_indexBuffer;
    uint32_t m_indexCount = 0;

    FastNoiseLite m_noise;
    BlockType m_blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
};
