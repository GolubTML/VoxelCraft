#include <core/frustum.hpp>

float Plane::getDistance(const glm::vec3& point) const
{
    return glm::dot(normal, point) + distance;
}

void Frustum::update(const glm::mat4& vp)
{
    // all of this code, is to get all faces of mesh

    planes[0].normal.x = vp[0][3] + vp[0][0];
    planes[0].normal.y = vp[1][3] + vp[1][0];
    planes[0].normal.z = vp[2][3] + vp[2][0];
    planes[0].distance = vp[3][3] + vp[3][0];

    planes[1].normal.x = vp[0][3] - vp[0][0];
    planes[1].normal.y = vp[1][3] - vp[1][0];
    planes[1].normal.z = vp[2][3] - vp[2][0];
    planes[1].distance = vp[3][3] - vp[3][0];

    planes[2].normal.x = vp[0][3] + vp[0][1];
    planes[2].normal.y = vp[1][3] + vp[1][1];
    planes[2].normal.z = vp[2][3] + vp[2][1];
    planes[2].distance = vp[3][3] + vp[3][1];

    planes[3].normal.x = vp[0][3] - vp[0][1];
    planes[3].normal.y = vp[1][3] - vp[1][1];
    planes[3].normal.z = vp[2][3] - vp[2][1];
    planes[3].distance = vp[3][3] - vp[3][1];

    planes[4].normal.x = vp[0][3] + vp[0][2];
    planes[4].normal.y = vp[1][3] + vp[1][2];
    planes[4].normal.z = vp[2][3] + vp[2][2];
    planes[4].distance = vp[3][3] + vp[3][2];

    planes[5].normal.x = vp[0][3] - vp[0][2];
    planes[5].normal.y = vp[1][3] - vp[1][2];
    planes[5].normal.z = vp[2][3] - vp[2][2];
    planes[5].distance = vp[3][3] - vp[3][2];

    for (auto& plane : planes)
    {
        float length = glm::length(plane.normal);

        plane.normal /= length;
        plane.distance /= length;
    }
}

bool Frustum::isBoxVisible(const glm::vec3& min, const glm::vec3& max) const
{
    for (const auto& plane : planes)
    {
        glm::vec3 pVertex = min;
        if (plane.normal.x >= 0) pVertex.x = max.x;
        if (plane.normal.y >= 0) pVertex.y = max.y;
        if (plane.normal.z >= 0) pVertex.z = max.z;

        if (plane.getDistance(pVertex) < 0)
            return false;
    }

    return true;
}