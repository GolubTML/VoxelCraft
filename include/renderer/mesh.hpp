#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <core/buffer.hpp>

class Device;
class GarbageCollector;

struct Vertex
{
    glm::vec3 pos;
    uint32_t color;
    glm::vec2 uvPos;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescription();

    static uint32_t packColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    static uint32_t applyColorFactor(uint32_t color, int factor);
};

class Mesh
{
public:
    void create(Device& device, 
        const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
        GarbageCollector& gc, uint32_t currFrame);
    
    void cleanup(VkDevice device);

    Buffer vertexBuffer;
    Buffer indexBuffer;

    uint32_t indexCount;
}; 