#pragma once

#include <game/block.hpp>
#include <glm/glm.hpp>
#include <renderer/mesh.hpp>

class Device;

class Chunk
{
public:
    static constexpr int WIDTH = 16;
    static constexpr int HEIGHT = 64;
    static constexpr int LENGTH = 16;

    Block blocks[WIDTH][HEIGHT][LENGTH];
    glm::ivec3 pos = glm::ivec3(0.f);

    void cleanup(VkDevice device);
    
    glm::mat4 modelMatrix;

    Mesh mesh;
};