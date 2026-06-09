#pragma once

#include <core/camera.hpp>
#include <GLFW/glfw3.h>
#include <game/block.hpp>
#include <memory>

// maybe, this struct shouldn't be here
struct RaycastResult
{
    bool hit = false;
    glm::ivec3 blockPos{0}; 
    glm::ivec3 normal{0};
};

class World; 

class Player
{
public:
    Player(glm::vec3 pos, float speed, float fov, uint32_t windowWidth, uint32_t windowHeight);
    ~Player();

    void update(GLFWwindow* window, float deltaTime, World& world);
    void handleMouse(GLFWwindow* window, double xpos, double ypos);

    Camera& getPlayerCamera() const;
    glm::vec3& getPlayerPosition();
    BlockType getCurrentBlock() const;
    
    const RaycastResult& getCurrentRaycast() const;

private:
    std::unique_ptr<Camera> playerCamera;
    glm::vec3 position;
    BlockType currentBlock;

    RaycastResult currentRay;
    
    float speed;
    float fov; // maybe, i will move this field to settings in future
    // float sensivity; // and this too

    void input(GLFWwindow* window, float deltaTime);

    RaycastResult raycast(const World& world, glm::vec3& origin, glm::vec3& dir, float dist);
};