#include <core/camera.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Camera::Camera(glm::vec3 position, float fov, float w, float h) 
    : pos(position), FOV(fov), size(w, h) 
{
    front = glm::vec3(0.f, 0.f, -1.f);
    up = glm::vec3(0.f, 1.f, 0.f);
}

Camera::~Camera() { }

void Camera::move(GLFWwindow* window, float deltaTime)
{
    float velocity = 1.f * deltaTime;

    glm::vec3 right = glm::normalize(glm::cross(front, up));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        pos += front * velocity;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        pos -= front * velocity;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        pos -= right * velocity;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        pos += right * velocity;
}

void Camera::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xoffset *= sens;
    yoffset *= sens;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.f)
        pitch = 89.f;
    if (pitch < -89.f)
        pitch = -89.f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(direction);
}

glm::mat4 Camera::getCameraView() const
{
    return glm::lookAt(pos, front + pos, up);
}

glm::mat4 Camera::getCameraProjection() const
{
    return glm::perspective(glm::radians(FOV), size.x / size.y, 0.1f, 100.f);
}