#pragma once

#include <core/camera.hpp>
#include <GLFW/glfw3.h>
#include <memory>

class Player
{
public:
    Player(glm::vec3 pos, float speed, float fov, uint32_t windowWidth, uint32_t windowHeight);
    ~Player();

    void update(GLFWwindow* window, float deltaTime);
    void handleMouse(GLFWwindow* window, double xpos, double ypos);

    Camera& getPlayerCamera() const;
    glm::vec3& getPlayerPosition();

private:
    std::unique_ptr<Camera> playerCamera;
    glm::vec3 position;
    
    float speed;
    float fov; // maybe, i will move this field to settings in future
    // float sensivity; // and this too

    void input(GLFWwindow* window, float deltaTime);
};