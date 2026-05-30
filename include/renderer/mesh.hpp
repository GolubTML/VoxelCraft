#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <core/buffer.hpp>

class Device;

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription();

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescription();
};

class Mesh
{
public:
    void create(Device& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    void cleanup(VkDevice device);

    Buffer vertexBuffer;
    Buffer indexBuffer;

    uint32_t indexCount;
}; 