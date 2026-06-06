#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <core/buffer.hpp>

class Device;
class GarbageCollector;

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 uvPos;

    static VkVertexInputBindingDescription getBindingDescription();

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescription();
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