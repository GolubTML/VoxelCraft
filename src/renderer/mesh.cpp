#include <renderer/mesh.hpp>

VkVertexInputBindingDescription Vertex::getBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription{};

    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> Vertex::getAttributeDescription()
{
    // so here, we will desctiption of how we need to cast our fields to shader
    // (as i undestand)

    std::array<VkVertexInputAttributeDescription, 2> attributeDescription{};
    // for position
    attributeDescription[0].binding = 0;
    attributeDescription[0].location = 0; // vertex position
    attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescription[0].offset = offsetof(Vertex, pos);
    // for color
    attributeDescription[1].binding = 0;
    attributeDescription[1].location = 1; // color
    attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescription[1].offset = offsetof(Vertex, color);

    return attributeDescription;
}