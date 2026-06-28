#pragma once
#include <array>

#include <glm/glm.hpp>

namespace voxl
{
    struct Plane
    {
        glm::vec3 normal = {0.0f, 1.0f, 0.0f};
        float distance   = 0.0f;

        Plane() = default;

        explicit Plane(const glm::vec4& p)
        {
            normal        = glm::vec3(p);
            const float l = glm::length(normal);
            normal        /= l;
            distance      = p.w / l;
        }
    };

    class Frustum
    {
    public:
        Frustum() = default;
        explicit Frustum(const glm::mat4& vpMatrix);

        void extractPlanes(const glm::mat4& vpMatrix);
        [[nodiscard]] bool intersectsAABB(const glm::vec3& minAABB, const glm::vec3& maxAABB) const;

    private:
        std::array<Plane, 6> m_planes;
    };
} // namespace voxl
