#pragma once

#include <vulkan/vulkan.h>

namespace vkUtils
{
    uint32_t findMemoryType(VkPhysicalDevice pDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
}