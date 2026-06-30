#include "Frustum.h"

#include <algorithm>

namespace voxl
{
    Frustum::Frustum(const glm::mat4& vpMatrix) { extractPlanes(vpMatrix); }

    void Frustum::extractPlanes(const glm::mat4& vpMatrix)
    {
        glm::mat4 M = glm::transpose(vpMatrix);
        m_planes[0] = Plane(M[3] + M[0]); // Left
        m_planes[1] = Plane(M[3] - M[0]); // Right
        m_planes[2] = Plane(M[3] + M[1]); // Bottom
        m_planes[3] = Plane(M[3] - M[1]); // Top
        m_planes[4] = Plane(M[3] + M[2]); // Near
        m_planes[5] = Plane(M[3] - M[2]); // Far
    }

    bool Frustum::intersectsAABB(const glm::vec3& minAABB, const glm::vec3& maxAABB) const
    {
        return std::ranges::all_of(m_planes, [&](const auto& plane)
        {
            glm::vec3 pVertex = minAABB;
            if (plane.normal.x >= 0) pVertex.x = maxAABB.x;
            if (plane.normal.y >= 0) pVertex.y = maxAABB.y;
            if (plane.normal.z >= 0) pVertex.z = maxAABB.z;

            return glm::dot(plane.normal, pVertex) + plane.distance >= 0.0f;
        });
    }
} // namespace voxl
