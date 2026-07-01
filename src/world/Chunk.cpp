#include "Chunk.h"

#include <array>
#include <fstream>
#include <mdspan>
#include <ranges>
#include <utility>

#include <FastNoiseLite/FastNoiseLite.h>

namespace voxl
{
    static constexpr std::array<std::array<glm::vec3, 4>, 6> FACE_VERTICES = {
        {
            // Front
            {{{-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}}},
            // Back
            {{{0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}}},
            // Left
            {{{-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, -0.5f}}},
            // Right
            {{{0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}}},
            // Top
            {{{-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}}},
            // Bottom
            {{{-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}}}
        }
    };


    Chunk::Chunk(const glm::vec3 position, MegaBuffer* vb, MegaBuffer* ib)
        : m_position(position), m_megaVertexBuffer(vb), m_megaIndexBuffer(ib)
    {
        m_blocks.fill(BlockType::Air);

        m_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        m_noise.SetFrequency(0.003f); // Bardzo niska częstotliwość dla długich, łagodnych wzgórz
    }

    Chunk::~Chunk()
    {
        if (m_vertexAllocation.has_value()) m_megaVertexBuffer->free(*m_vertexAllocation);
        if (m_indexAllocation.has_value()) m_megaIndexBuffer->free(*m_indexAllocation);
    }

    void Chunk::generateTerrain()
    {
        auto coords = std::views::cartesian_product(std::views::iota(0, static_cast<int>(CHUNK_SIZE)),
                                                    std::views::iota(0, static_cast<int>(CHUNK_SIZE)));
        const std::mdspan<Block, std::extents<size_t, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE>> span(m_blocks.data());

        for (const auto [x,z]: coords)
        {
            const float globalX = m_position.x + static_cast<float>(x);
            const float globalZ = m_position.z + static_cast<float>(z);

            const float noiseValue  = m_noise.GetNoise(globalX, globalZ);
            const int terrainHeight = static_cast<int>((noiseValue + 1.0f) * 0.5f * 60.0f) + 15;

            for (const auto y: std::views::iota(0ULL, CHUNK_SIZE))
            {
                if (const int globalY = static_cast<int>(std::round(m_position.y)) + static_cast<int>(y); globalY == terrainHeight - 1)
                {
                    span[x, y, z] = BlockType::Grass;
                }
                else if (globalY > terrainHeight - 4 && globalY < terrainHeight)
                {
                    span[x, y, z] = BlockType::Dirt;
                }
                else if (globalY < terrainHeight)
                {
                    span[x, y, z] = BlockType::Stone;
                }
                else
                {
                    span[x, y, z] = BlockType::Air;
                }
            }
        }


        m_isDirty     = true;
        m_isSaveDirty = true;
    }

    void Chunk::buildMesh()
    {
        if (m_vertexAllocation.has_value())
        {
            m_megaVertexBuffer->free(*m_vertexAllocation);
            m_vertexAllocation.reset();
        }
        if (m_indexAllocation.has_value())
        {
            m_megaIndexBuffer->free(*m_indexAllocation);
            m_indexAllocation.reset();
        }

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        for (int d = 0; d < 3; ++d)
        {
            int i, j, k, l, w, h;
            int u        = (d + 1) % 3;
            int v        = (d + 2) % 3;
            std::array x = {0, 0, 0};
            std::array q = {0, 0, 0};
            q[d]         = 1;

            std::array<int, 3> dims = {CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE};
            int dim_d               = dims[d];
            int dim_u               = dims[u];
            int dim_v               = dims[v];

            struct MaskCell
            {
                Block block;
                bool backFace{};

                bool operator==(const MaskCell& other) const
                {
                    return block == other.block && backFace == other.backFace;
                }

                bool operator!=(const MaskCell& other) const
                {
                    return !(*this == other);
                }
            };

            std::array<std::array<MaskCell, CHUNK_SIZE>, CHUNK_SIZE> mask{};

            for (x[d] = -1; x[d] < dim_d; ++x[d])
            {
                // Compute Mask
                for (x[v] = 0; x[v] < dim_v; ++x[v])
                {
                    for (x[u] = 0; x[u] < dim_u; ++x[u])
                    {
                        Block blockCurrent = getBlock(x[0], x[1], x[2]);
                        Block blockCompare = getBlock(x[0] + q[0], x[1] + q[1], x[2] + q[2]);

                        bool currentSolid = blockCurrent != BlockType::Air;
                        bool compareSolid = blockCompare != BlockType::Air;

                        if (currentSolid != compareSolid)
                        {
                            bool isBackFace  = !currentSolid;
                            mask[x[v]][x[u]] = {
                                .block = currentSolid ? blockCurrent : blockCompare, .backFace = isBackFace
                            };
                        }
                        else
                        {
                            mask[x[v]][x[u]] = {.block = BlockType::Air, .backFace = false};
                        }
                    }
                }

                // Generate Mesh from Mask
                for (j = 0; j < dim_v; ++j)
                {
                    for (i = 0; i < dim_u;)
                    {
                        MaskCell c = mask[j][i];
                        if (c.block != BlockType::Air)
                        {
                            for (w = 1; i + w < dim_u && mask[j][i + w] == c; ++w) {}

                            bool done = false;
                            for (h = 1; j + h < dim_v; ++h)
                            {
                                for (k = 0; k < w; ++k)
                                {
                                    if (mask[j + h][i + k] != c)
                                    {
                                        done = true;
                                        break;
                                    }
                                }
                                if (done) break;
                            }

                            x[u] = i;
                            x[v] = j;

                            std::array du = {0, 0, 0};
                            du[u]         = w;
                            std::array dv = {0, 0, 0};
                            dv[v]         = h;

                            glm::vec3 v1(x[0], x[1], x[2]);
                            v1[d] += 1.0f;
                            glm::vec3 v2(x[0] + du[0], x[1] + du[1], x[2] + du[2]);
                            v2[d] += 1.0f;
                            glm::vec3 v3(x[0] + du[0] + dv[0], x[1] + du[1] + dv[1], x[2] + du[2] + dv[2]);
                            v3[d] += 1.0f;
                            glm::vec3 v4(x[0] + dv[0], x[1] + dv[1], x[2] + dv[2]);
                            v4[d] += 1.0f;

                            glm::vec3 offset = m_position - glm::vec3(0.5f);
                            v1               += offset;
                            v2               += offset;
                            v3               += offset;
                            v4               += offset;

                            int faceIndex = 0;
                            if (d == 0) faceIndex = c.backFace ? 2 : 3;
                            else if (d == 1) faceIndex = c.backFace ? 5 : 4;
                            else faceIndex             = c.backFace ? 1 : 0;

                            glm::vec3 color;
                            switch (c.block.type)
                            {
                                case BlockType::Grass: color = {0.2f, 0.8f, 0.2f};
                                    break;
                                case BlockType::Dirt: color = {0.5f, 0.3f, 0.1f};
                                    break;
                                case BlockType::Stone: color = {0.5f, 0.5f, 0.5f};
                                    break;
                                case BlockType::Water: color = {0.2f, 0.4f, 0.9f};
                                    break;
                                case BlockType::Air: color = {1.0f, 1.0f, 1.0f};
                                    break;
                            }

                            if (faceIndex == 0 || faceIndex == 1) color *= 0.8f;
                            else if (faceIndex == 2 || faceIndex == 3) color *= 0.6f;
                            else if (faceIndex == 5) color *= 0.4f;

                            auto startIndex = static_cast<uint32_t>(vertices.size());

                            if (!c.backFace)
                            {
                                vertices.push_back(Vertex{.pos = v1, .color = color});
                                vertices.push_back(Vertex{.pos = v2, .color = color});
                                vertices.push_back(Vertex{.pos = v3, .color = color});
                                vertices.push_back(Vertex{.pos = v4, .color = color});
                            }
                            else
                            {
                                vertices.push_back(Vertex{.pos = v1, .color = color});
                                vertices.push_back(Vertex{.pos = v4, .color = color});
                                vertices.push_back(Vertex{.pos = v3, .color = color});
                                vertices.push_back(Vertex{.pos = v2, .color = color});
                            }

                            indices.insert(indices.end(), {
                                               startIndex + 0, startIndex + 1, startIndex + 2,
                                               startIndex + 2, startIndex + 3, startIndex + 0
                                           });

                            for (l = 0; l < h; ++l)
                            {
                                for (k = 0; k < w; ++k)
                                {
                                    mask[j + l][i + k] = {.block = BlockType::Air, .backFace = false};
                                }
                            }

                            i += w;
                        }
                        else
                        {
                            ++i;
                        }
                    }
                }
            }
        }

        m_indexCount = static_cast<uint32_t>(indices.size());

        if (m_indexCount > 0)
        {
            auto vertexSize    = static_cast<uint32_t>(sizeof(Vertex) * vertices.size());
            m_vertexAllocation = m_megaVertexBuffer->allocate(vertexSize)
                                                    .and_then([&](const BlockAllocation& alloc) -> std::optional<BlockAllocation>
                                                     {
                                                         m_megaVertexBuffer->upload(alloc, std::span<const Vertex>{vertices});
                                                         return alloc;
                                                     });

            auto indexSize    = static_cast<uint32_t>(sizeof(indices[0]) * indices.size());
            m_indexAllocation = m_megaIndexBuffer->allocate(indexSize)
                                                  .and_then([&](const BlockAllocation& alloc) -> std::optional<BlockAllocation>
                                                   {
                                                       m_megaIndexBuffer->upload(alloc, std::span<const uint32_t>{indices});
                                                       return alloc;
                                                   });
        }
    }

    bool Chunk::setBlock(int x, int y, int z, Block block)
    {
        if (x >= 0 && std::cmp_less(x, CHUNK_SIZE) && y >= 0 && std::cmp_less(y, CHUNK_SIZE) && z >= 0 && std::cmp_less(z, CHUNK_SIZE))
        {
            const std::mdspan<Block, std::extents<size_t, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE>> span(m_blocks.data());
            if (span[x, y, z].type != block.type || span[x, y, z].metadata != block.metadata)
            {
                span[x, y, z] = block;
                m_isDirty     = true;
                m_isSaveDirty = true;
                return true;
            }
        }
        return false;
    }

    Block Chunk::getBlock(const int x, const int y, const int z) const
    {
        if (x >= 0 && std::cmp_less(x, CHUNK_SIZE) && y >= 0 && std::cmp_less(y, CHUNK_SIZE) && z >= 0 && std::cmp_less(z, CHUNK_SIZE))
        {
            const std::mdspan<const Block, std::extents<size_t, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE>> span(m_blocks.data());
            return span[x, y, z];
        }

        const float globalX = m_position.x + static_cast<float>(x);
        const int globalY   = static_cast<int>(std::round(m_position.y)) + y;
        const float globalZ = m_position.z + static_cast<float>(z);

        if (globalY < 0) return BlockType::Stone;

        const float noiseValue  = m_noise.GetNoise(globalX, globalZ);
        const int terrainHeight = static_cast<int>((noiseValue + 1.0f) * 0.5f * 60.0f) + 15;

        if (globalY >= terrainHeight) return BlockType::Air;
        if (globalY == terrainHeight - 1) return BlockType::Grass;
        if (globalY >= terrainHeight - 4) return BlockType::Dirt;
        return BlockType::Stone;
    }

    bool Chunk::save(const std::string& filepath)
    {
        if (!m_isSaveDirty) return true;
        m_isSaveDirty = false;

        std::ofstream out(filepath, std::ios::binary);
        if (!out) return false;

        BlockType currentType = m_blocks[0].type;
        uint32_t count        = 0;

        for (const auto& block: m_blocks)
        {
            if (block.type == currentType)
            {
                count++;
            }
            else
            {
                out.write(reinterpret_cast<const char*>(&currentType), sizeof(BlockType));
                out.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));
                currentType = block.type;
                count       = 1;
            }
        }
        out.write(reinterpret_cast<const char*>(&currentType), sizeof(BlockType));
        out.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));

        m_isDirty = false;
        return true;
    }

    bool Chunk::load(const std::string& filepath)
    {
        std::ifstream in(filepath, std::ios::binary);
        if (!in) return false;

        size_t index = 0;
        while (in && index < m_blocks.size())
        {
            BlockType type;
            uint32_t count;
            in.read(reinterpret_cast<char*>(&type), sizeof(BlockType));
            if (in.eof()) break;
            in.read(reinterpret_cast<char*>(&count), sizeof(uint32_t));

            for (uint32_t i = 0; i < count && index < m_blocks.size(); ++i)
            {
                m_blocks[index++] = {type, 0};
            }
        }
        m_isDirty = false;
        return true;
    }

    bool Chunk::isFaceVisible(int x, int y, int z) const
    {
        const BlockType type = getBlock(x, y, z);
        return type == BlockType::Air;
    }

    void Chunk::addFace(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int x, int y, int z, int faceIndex,
                        Block block) const
    {
        auto startIndex = static_cast<uint32_t>(vertices.size());

        glm::vec3 color;
        switch (block.type)
        {
            case BlockType::Grass: color = {0.2f, 0.8f, 0.2f};
                break;
            case BlockType::Dirt: color = {0.5f, 0.3f, 0.1f};
                break;
            case BlockType::Stone: color = {0.5f, 0.5f, 0.5f};
                break;
            case BlockType::Water: color = {0.2f, 0.4f, 0.9f};
                break;
            case BlockType::Air: color = {1.0f, 1.0f, 1.0f};
                break;
        }

        if (faceIndex == 0 || faceIndex == 1) color *= 0.8f;
        else if (faceIndex == 2 || faceIndex == 3) color *= 0.6f;
        else if (faceIndex == 5) color *= 0.4f;

        for (int i = 0; i < 4; ++i)
        {
            Vertex v{};
            v.pos   = FACE_VERTICES[faceIndex][i] + glm::vec3(x, y, z) + m_position;
            v.color = color;
            vertices.push_back(v);
        }

        indices.insert(indices.end(), {
                           startIndex + 0, startIndex + 1, startIndex + 2,
                           startIndex + 2, startIndex + 3, startIndex + 0
                       });
    }
} // namespace voxl
