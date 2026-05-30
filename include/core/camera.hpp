#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class Camera
{
public:
    glm::vec3 pos = glm::vec3(0.f, 0.f, 0.f);
    glm::vec3 front = glm::vec3(0.f, 0.f, 0.f);
    glm::vec3 up = glm::vec3(0.f, 0.f, 0.f);

    float sens = 0.2f;
    float FOV = 0.f;
    glm::vec2 size = glm::vec2(0.f, 0.f);

    Camera() { }
    Camera(glm::vec3 position, float fov, float w, float h);
    ~Camera();

    void move(GLFWwindow* window, float deltaTime);
    void mouse_callback(GLFWwindow* window, double xpos, double ypos);

    glm::mat4 getCameraView() const;
    glm::mat4 getCameraProjection() const;

private:
    bool firstMouse = true;
    float lastX = 0.f;
    float lastY = 0.f;

    float yaw = -90.f;
    float pitch = 0.f;
};