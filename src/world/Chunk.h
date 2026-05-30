#pragma once
#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "../renderer/Buffer.h"
#include "../renderer/Vertex.h"
#include "../../vendor/FastNoiseLite.h"
#include "../renderer/MegaBuffer.h"
#include "../renderer/GraphicsPipeline.h"

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
    static constexpr int CHUNK_SIZE = 64;

    Chunk(glm::vec3 position, MegaBuffer* vb, MegaBuffer* ib);
    ~Chunk();

    void generateTerrain();
    void buildMesh();

    void setBlock(int x, int y, int z, BlockType type);
    BlockType getBlock(int x, int y, int z) const;
    
    [[nodiscard]] uint32_t getIndexCount() const { return m_indexCount; }
    [[nodiscard]] glm::vec3 getPosition() const { return m_position; }

    bool save(const std::string& filepath);
    bool load(const std::string& filepath);
    void setDirty(bool dirty) { m_isDirty = dirty; }
    bool isDirty() const { return m_isDirty; }

    bool hasValidAllocation() const { return m_vertexAllocation.valid && m_indexAllocation.valid; }
    uint32_t getVertexOffset() const { return m_vertexAllocation.offset; }
    uint32_t getIndexOffset() const { return m_indexAllocation.offset; }

private:
    bool isFaceVisible(int x, int y, int z) const;
    void addFace(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int x, int y, int z, int faceIndex, BlockType type);

    glm::vec3 m_position;
    
    MegaBuffer* m_megaVertexBuffer;
    MegaBuffer* m_megaIndexBuffer;
    BlockAllocation m_vertexAllocation;
    BlockAllocation m_indexAllocation;

    uint32_t m_indexCount = 0;

    FastNoiseLite m_noise;
    BlockType m_blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
    bool m_isDirty = false;
};
