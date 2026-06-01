#pragma once

#include <vulkan/vulkan.h>

class Buffer
{
public:
    VkBuffer buffer;
    VkDeviceMemory memory;

    void create(VkPhysicalDevice pDevice, VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlagBits bufferFlags, const void* data);
    void cleanup(VkDevice device);
};