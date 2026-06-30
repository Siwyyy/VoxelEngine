#pragma once
#include <array>

#include "Vertex.h"

namespace voxl
{
    class VoxelMesh
    {
    public:
        // Sześcian o wymiarach 1x1x1 ze środkiem w punkcie (0,0,0)
        static constexpr std::array<Vertex, 8> vertices = {
            {
                // Przód
                {.pos = {-0.5f, -0.5f, 0.5f}, .color = {1.0f, 0.0f, 0.0f}},
                {.pos = {0.5f, -0.5f, 0.5f}, .color = {0.0f, 1.0f, 0.0f}},
                {.pos = {0.5f, 0.5f, 0.5f}, .color = {0.0f, 0.0f, 1.0f}},
                {.pos = {-0.5f, 0.5f, 0.5f}, .color = {1.0f, 1.0f, 1.0f}},

                // Tył
                {.pos = {-0.5f, -0.5f, -0.5f}, .color = {1.0f, 0.0f, 0.0f}},
                {.pos = {0.5f, -0.5f, -0.5f}, .color = {0.0f, 1.0f, 0.0f}},
                {.pos = {0.5f, 0.5f, -0.5f}, .color = {0.0f, 0.0f, 1.0f}},
                {.pos = {-0.5f, 0.5f, -0.5f}, .color = {1.0f, 1.0f, 1.0f}}
            }
        };

        static constexpr std::array<uint16_t, 36> indices = {
            {
                0, 1, 2, 2, 3, 0, // Przód
                1, 5, 6, 6, 2, 1, // Prawa
                5, 4, 7, 7, 6, 5, // Tył
                4, 0, 3, 3, 7, 4, // Lewa
                3, 2, 6, 6, 7, 3, // Góra
                4, 5, 1, 1, 0, 4 // Dół
            }
        };
    };
} // namespace voxl
