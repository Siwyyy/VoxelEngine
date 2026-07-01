#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include <FastNoiseLite/FastNoiseLite.h>
#include <glm/glm.hpp>

#include "renderer/MegaBuffer.h"
#include "renderer/Vertex.h"

namespace voxl
{
    enum class BlockType : uint8_t
    {
        Air   = 0,
        Grass = 1,
        Dirt  = 2,
        Stone = 3,
        Water = 6
    };

    struct Block
    {
        BlockType type;
        uint8_t metadata;

        Block() : type(BlockType::Air), metadata(0) {}
        Block(const BlockType t, const uint8_t m = 0) : type(t), metadata(m) {} // NOLINT(google-explicit-constructor)

        bool operator==(const Block& other) const { return type == other.type && metadata == other.metadata; }
        bool operator!=(const Block& other) const { return !(*this == other); }
        bool operator==(const BlockType t) const { return type == t; }
        bool operator!=(const BlockType t) const { return type != t; }

        operator BlockType() const { return type; }
    };

    class Chunk
    {
    public:
        static constexpr size_t CHUNK_SIZE = 64;

        Chunk(glm::vec3 position, MegaBuffer* vb, MegaBuffer* ib);
        Chunk(Chunk&&)            = delete;
        Chunk& operator=(Chunk&&) = delete;
        ~Chunk();

        void generateTerrain();
        void buildMesh();

        [[nodiscard]] glm::vec3 getPosition() const { return m_position; }
        [[nodiscard]] uint32_t getIndexCount() const { return m_indexCount; }

        bool setBlock(int x, int y, int z, Block block);
        [[nodiscard]] Block getBlock(int x, int y, int z) const;

        bool save(const std::string& filepath);
        bool load(const std::string& filepath);
        void setDirty(const bool dirty) { m_isDirty = dirty; }
        void setSaveDirty(const bool dirty) { m_isSaveDirty = dirty; }
        [[nodiscard]] bool isDirty() const { return m_isDirty; }
        [[nodiscard]] bool isSaveDirty() const { return m_isSaveDirty; }

        [[nodiscard]] bool hasValidAllocation() const { return m_vertexAllocation.has_value() && m_indexAllocation.has_value(); }
        [[nodiscard]] uint32_t getVertexOffset() const { return m_vertexAllocation->offset; }
        [[nodiscard]] uint32_t getIndexOffset() const { return m_indexAllocation->offset; }

    private:
        glm::vec3 m_position;

        MegaBuffer* m_megaVertexBuffer;
        MegaBuffer* m_megaIndexBuffer;
        std::optional<BlockAllocation> m_vertexAllocation;
        std::optional<BlockAllocation> m_indexAllocation;

        uint32_t m_indexCount = 0;

        FastNoiseLite m_noise;
        std::array<Block, CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE> m_blocks{};

        bool m_isDirty     = false;
        bool m_isSaveDirty = false;

        [[nodiscard]] bool isFaceVisible(int x, int y, int z) const;

        void addFace(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int x, int y, int z, int faceIndex,
                     Block block) const;
    };
} // namespace voxl
