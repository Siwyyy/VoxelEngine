#pragma once
#include "Vertex.h"
#include <vector>

class VoxelMesh {
public:
    static const std::vector<Vertex> vertices;
    static const std::vector<uint16_t> indices;
};
