#pragma once
#include <vector>

#include "Vertex.h"

class VoxelMesh
{
public:
    static const std::vector<Vertex> vertices;
    static const std::vector<uint16_t> indices;
};
