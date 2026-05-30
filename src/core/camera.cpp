#include <core/camera.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Camera::Camera(glm::vec3 position, float fov, float w, float h) 
    : pos(position), FOV(fov), size(w, h) 
{
}

Camera::~Camera() { }

glm::mat4 Camera::getCameraView() const
{
    return glm::lookAt(pos, front, up);
}

glm::mat4 Camera::getCameraProjection() const
{
    return glm::perspective(glm::radians(FOV), size.x / size.y, 0.1f, 100.f);
}