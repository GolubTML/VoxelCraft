#include <core/buffer.hpp>
#include <stdexcept>
#include <cstring>

void Buffer::create(VkPhysicalDevice pDevice, VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlagBits bufferFlags, const void* data)
{
    // firstly, create info
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = bufferFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // creating
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create buffer!");
    }

    // and now, memory requirements
    VkMemoryRequirements memRequirements;   
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    // allocate info
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(pDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // and allocate
    if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot allocate memory for vertex buffer!");
    }

    // now, bind it
    vkBindBufferMemory(device, buffer, memory, 0);

    // here, copy from RAM to VRAM
    if (data != nullptr)
    {
        void* mapppedData;
        vkMapMemory(device, memory, 0, bufferInfo.size, 0, &mapppedData);
        memcpy(mapppedData, data, (size_t) bufferInfo.size);
        vkUnmapMemory(device, memory);
    }
}

void Buffer::cleanup(VkDevice device)
{
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);
}

uint32_t Buffer::findMemoryType(VkPhysicalDevice pDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    // and here, we need memory properties
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(pDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}