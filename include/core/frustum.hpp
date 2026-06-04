#pragma once

#include <array>
#include <glm/glm.hpp>

struct Plane
{
    glm::vec3 normal;
    float distance;

    float getDistance(const glm::vec3& point) const;
};

class Frustum
{
public:
    std::array<Plane, 6> planes;

    void update(const glm::mat4& vp);

    bool isBoxVisible(const glm::vec3& min, const glm::vec3& max) const;
};