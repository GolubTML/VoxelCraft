#include <core/garbageCollector.hpp>

void GarbageCollector::pushBuffer(VkBuffer buffer, VkDeviceMemory memory, uint32_t inFlightFrameIndex)
{
    buffersToDelete[inFlightFrameIndex].push_back({buffer, memory});
}

void GarbageCollector::cleanupFrame(VkDevice device, uint32_t inFlightFrameIndex)
{
    for (auto& b : buffersToDelete[inFlightFrameIndex]) 
    {
        vkDestroyBuffer(device, b.buffer, nullptr);
        vkFreeMemory(device, b.memory, nullptr);
    }

    buffersToDelete[inFlightFrameIndex].clear();
}

void GarbageCollector::cleanup(VkDevice device) 
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
    {
        for (auto& b : buffersToDelete[i]) 
        {
            vkDestroyBuffer(device, b.buffer, nullptr);
            vkFreeMemory(device, b.memory, nullptr);
        }
        buffersToDelete[i].clear();
    }
}