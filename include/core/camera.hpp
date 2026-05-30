#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
    glm::vec3 pos = glm::vec3(0.f, 0.f, 0.f);
    glm::vec3 front = glm::vec3(0.f, 0.f, 0.f);
    glm::vec3 up = glm::vec3(0.f, 0.f, 1.f);

    float FOV = 0.f;
    glm::vec2 size = glm::vec2(0.f, 0.f);


    Camera() { }
    Camera(glm::vec3 position, float fov, float w, float h);
    ~Camera();

    glm::mat4 getCameraView() const;
    glm::mat4 getCameraProjection() const;

private:
    float yaw = -90.f;
    float pitch = 0.f;
};