#pragma once

#include <game/block.hpp>
#include <glm/glm.hpp>
#include <renderer/mesh.hpp>

class Device;

class Chunk
{
public:
    static constexpr uint8_t WIDTH = 16;
    static constexpr uint8_t HEIGHT = 64;
    static constexpr uint8_t LENGTH = 16;

    bool isGenerated = false;
    bool needUpdate = false;

    Block blocks[WIDTH][HEIGHT][LENGTH];
    glm::ivec3 pos = glm::ivec3(0.f);

    void cleanup(VkDevice device);
    
    glm::mat4 modelMatrix;

    Mesh mesh;
};