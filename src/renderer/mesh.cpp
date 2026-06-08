#include <renderer/mesh.hpp>
#include <core/device.hpp>
#include <core/garbageCollector.hpp>

VkVertexInputBindingDescription Vertex::getBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription{};

    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescription()
{
    // so here, we will desctiption of how we need to cast our fields to shader
    // (as i undestand)

    std::array<VkVertexInputAttributeDescription, 3> attributeDescription{};
    // for position
    attributeDescription[0].binding = 0;
    attributeDescription[0].location = 0; // vertex position
    attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescription[0].offset = offsetof(Vertex, pos);
    // for color
    attributeDescription[1].binding = 0;
    attributeDescription[1].location = 1; // color
    attributeDescription[1].format = VK_FORMAT_R8G8B8A8_UNORM; // now, we pack color to one uint32_t
    attributeDescription[1].offset = offsetof(Vertex, color);
    // for uv
    attributeDescription[2].binding = 0;
    attributeDescription[2].location = 2; // uv coord
    attributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescription[2].offset = offsetof(Vertex, uvPos);

    return attributeDescription;
}

uint32_t Vertex::packColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (a << 24) | (b << 16) | (g << 8) | r;
}

void Mesh::create(Device& device, 
        const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
        GarbageCollector& gc, uint32_t currFrame)
{
    if (vertexBuffer.buffer != VK_NULL_HANDLE)
    {
        gc.pushBuffer(vertexBuffer.buffer, vertexBuffer.memory, currFrame);

        vertexBuffer.buffer = VK_NULL_HANDLE;
        vertexBuffer.memory = VK_NULL_HANDLE;
    }

    if (indexBuffer.buffer != VK_NULL_HANDLE)
    {
        gc.pushBuffer(indexBuffer.buffer, indexBuffer.memory, currFrame);
        
        indexBuffer.buffer = VK_NULL_HANDLE;
        indexBuffer.memory = VK_NULL_HANDLE;
    }

    VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();
    vertexBuffer.create(device.getPhysicalDevice(), device.getDevice(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices.data());

    VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();
    indexBuffer.create(device.getPhysicalDevice(), device.getDevice(), indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices.data());

    indexCount = indices.size();
}

void Mesh::cleanup(VkDevice device)
{
    vertexBuffer.cleanup(device);
    indexBuffer.cleanup(device);
}