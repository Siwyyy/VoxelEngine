#include "Chunk.h"
#include <array>
#include <algorithm>
#include <fstream>
#include "../../vendor/FastNoiseLite.h"

static const glm::vec3 FACE_VERTICES[6][4] = {
    // Front
    {{-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}},
    // Back
    {{0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}},
    // Left
    {{-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, -0.5f}},
    // Right
    {{0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}},
    // Top
    {{-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}},
    // Bottom
    {{-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}}
};

static const int FACE_CHECKS[6][3] = {
    {0, 0, 1}, // Front
    {0, 0, -1}, // Back
    {-1, 0, 0}, // Left
    {1, 0, 0}, // Right
    {0, 1, 0}, // Top
    {0, -1, 0} // Bottom
};

Chunk::Chunk(glm::vec3 position, MegaBuffer* vb, MegaBuffer* ib)
    : m_position(position), m_megaVertexBuffer(vb), m_megaIndexBuffer(ib)
{
    m_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_noise.SetFrequency(0.003f); // Bardzo niska częstotliwość dla długich, łagodnych wzgórz

    for (int x = 0; x < CHUNK_SIZE; ++x)
    {
        for (int y = 0; y < CHUNK_SIZE; ++y)
        {
            for (int z = 0; z < CHUNK_SIZE; ++z)
            {
                m_blocks[x][y][z] = BlockType::Air;
            }
        }
    }
}

Chunk::~Chunk()
{
    if (m_vertexAllocation.valid) {
        m_megaVertexBuffer->free(m_vertexAllocation);
    }
    if (m_indexAllocation.valid) {
        m_megaIndexBuffer->free(m_indexAllocation);
    }
}

void Chunk::generateTerrain()
{
    bool isAllAir = true;
    for (int x = 0; x < CHUNK_SIZE; ++x)
    {
        for (int z = 0; z < CHUNK_SIZE; ++z)
        {
            float globalX = m_position.x + x;
            float globalZ = m_position.z + z;

            float noiseValue = m_noise.GetNoise(globalX, globalZ);
            int terrainHeight = static_cast<int>((noiseValue + 1.0f) * 0.5f * 60.0f) + 15;
            
            for (int y = 0; y < CHUNK_SIZE; ++y)
            {
                float globalY = m_position.y + y;
                
                if (globalY == terrainHeight - 1)
                {
                    m_blocks[x][y][z] = BlockType::Grass;
                    isAllAir = false;
                }
                else if (globalY > terrainHeight - 4 && globalY < terrainHeight)
                {
                    m_blocks[x][y][z] = BlockType::Dirt;
                    isAllAir = false;
                }
                else if (globalY < terrainHeight)
                {
                    m_blocks[x][y][z] = BlockType::Stone;
                    isAllAir = false;
                }
                else
                {
                    m_blocks[x][y][z] = BlockType::Air;
                }
            }
        }
    }

    // Temporary basic tree generation just for testing 3D chunk boundary logic.
    // Full cross-chunk trees will be added next.
    int chunkX = static_cast<int>(std::floor(m_position.x / CHUNK_SIZE));
    int chunkZ = static_cast<int>(std::floor(m_position.z / CHUNK_SIZE));
    int hash = std::abs(chunkX * 73856093 ^ chunkZ * 19349663);

    if (hash % 4 == 0) 
    {
        int tx = 32;
        int tz = 32;
        
        // Find surface globally for this x,z
        float noiseValue = m_noise.GetNoise(m_position.x + tx, m_position.z + tz);
        int terrainHeight = static_cast<int>((noiseValue + 1.0f) * 0.5f * 60.0f) + 15;
        
        // Check if tree trunk should exist in this vertical chunk
        int treeHeight = 20 + (hash % 15);
        int leavesRadius = 8;
        
        for(int gy = terrainHeight; gy <= terrainHeight + treeHeight + leavesRadius; gy++) {
            if (gy >= m_position.y && gy < m_position.y + CHUNK_SIZE) {
                int ly = gy - static_cast<int>(m_position.y);
                
                if (gy < terrainHeight + treeHeight) {
                    // Trunk
                    m_blocks[tx][ly][tz] = BlockType::Wood;
                } else {
                    // Leaves
                    for (int dx = -leavesRadius; dx <= leavesRadius; ++dx) {
                        for (int dy = -leavesRadius; dy <= leavesRadius; ++dy) {
                            for (int dz = -leavesRadius; dz <= leavesRadius; ++dz) {
                                if (dx*dx + dy*dy + dz*dz <= leavesRadius*leavesRadius) {
                                    int lx = tx + dx;
                                    int lz = tz + dz;
                                    int check_ly = ly + dy;
                                    if (lx >= 0 && lx < CHUNK_SIZE && check_ly >= 0 && check_ly < CHUNK_SIZE && lz >= 0 && lz < CHUNK_SIZE) {
                                        if (m_blocks[lx][check_ly][lz] == BlockType::Air) {
                                            m_blocks[lx][check_ly][lz] = BlockType::Leaves;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    m_isDirty = true;
}
void Chunk::setBlock(int x, int y, int z, BlockType type)
{
    if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_SIZE && z >= 0 && z < CHUNK_SIZE)
    {
        m_blocks[x][y][z] = type;
        m_isDirty = true;
    }
}

BlockType Chunk::getBlock(int x, int y, int z) const
{
    if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_SIZE && z >= 0 && z < CHUNK_SIZE)
    {
        return m_blocks[x][y][z];
    }
    
    float globalX = m_position.x + x;
    float globalY = m_position.y + y;
    float globalZ = m_position.z + z;
    
    if (globalY < 0) return BlockType::Stone; 
    
    float noiseValue = m_noise.GetNoise(globalX, globalZ);
    int terrainHeight = static_cast<int>((noiseValue + 1.0f) * 0.5f * 60.0f) + 15;
    
    if (globalY >= terrainHeight) return BlockType::Air;
    if (globalY == terrainHeight - 1) return BlockType::Grass;
    if (globalY >= terrainHeight - 4) return BlockType::Dirt;
    return BlockType::Stone;
}

bool Chunk::isFaceVisible(int x, int y, int z) const
{
    BlockType type = getBlock(x, y, z);
    return type == BlockType::Air;
}

void Chunk::addFace(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int x, int y, int z, int faceIndex,
                    BlockType type)
{
    uint32_t startIndex = static_cast<uint32_t>(vertices.size());

    glm::vec3 color;
    switch (type)
    {
    case BlockType::Grass: color = {0.2f, 0.8f, 0.2f};
        break;
    case BlockType::Dirt: color = {0.5f, 0.3f, 0.1f};
        break;
    case BlockType::Stone: color = {0.5f, 0.5f, 0.5f};
        break;
    default: color = {1.0f, 1.0f, 1.0f};
        break;
    }

    if (faceIndex == 0 || faceIndex == 1) color *= 0.8f;
    else if (faceIndex == 2 || faceIndex == 3) color *= 0.6f;
    else if (faceIndex == 5) color *= 0.4f;

    for (int i = 0; i < 4; ++i)
    {
        Vertex v;
        v.pos = FACE_VERTICES[faceIndex][i] + glm::vec3(x, y, z) + m_position;
        v.color = color;
        vertices.push_back(v);
    }

    indices.push_back(startIndex + 0);
    indices.push_back(startIndex + 1);
    indices.push_back(startIndex + 2);
    indices.push_back(startIndex + 2);
    indices.push_back(startIndex + 3);
    indices.push_back(startIndex + 0);
}

void Chunk::buildMesh()
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (int d = 0; d < 3; ++d)
    {
        int i, j, k, l, w, h;
        int u = (d + 1) % 3;
        int v = (d + 2) % 3;
        int x[3] = {0, 0, 0};
        int q[3] = {0, 0, 0};
        q[d] = 1;
        
        int dims[3] = {CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE};
        int dim_d = dims[d];
        int dim_u = dims[u];
        int dim_v = dims[v];

        struct MaskCell {
            BlockType type;
            bool backFace;
            bool operator==(const MaskCell& other) const {
                return type == other.type && backFace == other.backFace;
            }
            bool operator!=(const MaskCell& other) const {
                return !(*this == other);
            }
        };

        MaskCell mask[256][256];

        for (x[d] = -1; x[d] < dim_d; ++x[d])
        {
            // Compute Mask
            for (x[v] = 0; x[v] < dim_v; ++x[v])
            {
                for (x[u] = 0; x[u] < dim_u; ++x[u])
                {
                    BlockType blockCurrent = getBlock(x[0], x[1], x[2]);
                    BlockType blockCompare = getBlock(x[0] + q[0], x[1] + q[1], x[2] + q[2]);

                    bool currentSolid = blockCurrent != BlockType::Air;
                    bool compareSolid = blockCompare != BlockType::Air;

                    if (currentSolid != compareSolid)
                    {
                        bool isBackFace = !currentSolid;
                        mask[x[v]][x[u]] = {currentSolid ? blockCurrent : blockCompare, isBackFace};
                    }
                    else
                    {
                        mask[x[v]][x[u]] = {BlockType::Air, false};
                    }
                }
            }

            // Generate Mesh from Mask
            for (j = 0; j < dim_v; ++j)
            {
                for (i = 0; i < dim_u; )
                {
                    MaskCell c = mask[j][i];
                    if (c.type != BlockType::Air)
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

                        int du[3] = {0, 0, 0}; du[u] = w;
                        int dv[3] = {0, 0, 0}; dv[v] = h;

                        glm::vec3 v1(x[0], x[1], x[2]); v1[d] += 1.0f;
                        glm::vec3 v2(x[0] + du[0], x[1] + du[1], x[2] + du[2]); v2[d] += 1.0f;
                        glm::vec3 v3(x[0] + du[0] + dv[0], x[1] + du[1] + dv[1], x[2] + du[2] + dv[2]); v3[d] += 1.0f;
                        glm::vec3 v4(x[0] + dv[0], x[1] + dv[1], x[2] + dv[2]); v4[d] += 1.0f;

                        glm::vec3 offset = m_position - glm::vec3(0.5f);
                        v1 += offset;
                        v2 += offset;
                        v3 += offset;
                        v4 += offset;

                        int faceIndex = 0;
                        if (d == 0) faceIndex = c.backFace ? 2 : 3;
                        else if (d == 1) faceIndex = c.backFace ? 5 : 4;
                        else faceIndex = c.backFace ? 1 : 0;

                        glm::vec3 color;
                        switch (c.type)
                        {
                        case BlockType::Grass: color = {0.2f, 0.8f, 0.2f}; break;
                        case BlockType::Dirt: color = {0.5f, 0.3f, 0.1f}; break;
                        case BlockType::Stone: color = {0.5f, 0.5f, 0.5f}; break;
                        case BlockType::Wood: color = {0.4f, 0.2f, 0.0f}; break;
                        case BlockType::Leaves: color = {0.1f, 0.6f, 0.1f}; break;
                        default: color = {1.0f, 1.0f, 1.0f}; break;
                        }

                        if (faceIndex == 0 || faceIndex == 1) color *= 0.8f;
                        else if (faceIndex == 2 || faceIndex == 3) color *= 0.6f;
                        else if (faceIndex == 5) color *= 0.4f;

                        uint32_t startIndex = static_cast<uint32_t>(vertices.size());
                        Vertex vert1, vert2, vert3, vert4;
                        vert1.color = color; vert2.color = color; vert3.color = color; vert4.color = color;

                        if (!c.backFace)
                        {
                            vert1.pos = v1; vert2.pos = v2; vert3.pos = v3; vert4.pos = v4;
                        }
                        else
                        {
                            vert1.pos = v1; vert2.pos = v4; vert3.pos = v3; vert4.pos = v2;
                        }

                        vertices.push_back(vert1);
                        vertices.push_back(vert2);
                        vertices.push_back(vert3);
                        vertices.push_back(vert4);

                        indices.push_back(startIndex + 0);
                        indices.push_back(startIndex + 1);
                        indices.push_back(startIndex + 2);
                        indices.push_back(startIndex + 2);
                        indices.push_back(startIndex + 3);
                        indices.push_back(startIndex + 0);

                        for (l = 0; l < h; ++l)
                        {
                            for (k = 0; k < w; ++k)
                            {
                                mask[j + l][i + k] = {BlockType::Air, false};
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
        uint32_t vertexSize = sizeof(Vertex) * vertices.size();
        m_vertexAllocation = m_megaVertexBuffer->allocate(vertexSize);
        if (m_vertexAllocation.valid) {
            m_megaVertexBuffer->upload(m_vertexAllocation, vertices.data());
        }

        uint32_t indexSize = sizeof(indices[0]) * indices.size();
        m_indexAllocation = m_megaIndexBuffer->allocate(indexSize);
        if (m_indexAllocation.valid) {
            m_megaIndexBuffer->upload(m_indexAllocation, indices.data());
        }
    }
}

bool Chunk::save(const std::string& filepath)
{
    if (!m_isDirty) return true;
    
    std::ofstream out(filepath, std::ios::binary);
    if (!out) return false;
    
    BlockType currentType = m_blocks[0][0][0];
    uint32_t count = 0;
    
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int y = 0; y < CHUNK_SIZE; ++y) {
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                if (m_blocks[x][y][z] == currentType) {
                    count++;
                } else {
                    out.write(reinterpret_cast<const char*>(&currentType), sizeof(BlockType));
                    out.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));
                    currentType = m_blocks[x][y][z];
                    count = 1;
                }
            }
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
    
    int x = 0, y = 0, z = 0;
    while (in && x < CHUNK_SIZE) {
        BlockType type;
        uint32_t count;
        in.read(reinterpret_cast<char*>(&type), sizeof(BlockType));
        if (in.eof()) break;
        in.read(reinterpret_cast<char*>(&count), sizeof(uint32_t));
        
        for (uint32_t i = 0; i < count; ++i) {
            m_blocks[x][y][z] = type;
            z++;
            if (z >= CHUNK_SIZE) {
                z = 0;
                y++;
                if (y >= CHUNK_SIZE) {
                    y = 0;
                    x++;
                    if (x >= CHUNK_SIZE) break;
                }
            }
        }
    }
    m_isDirty = false;
    return true;
}
