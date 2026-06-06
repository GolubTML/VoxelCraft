#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class GarbageCollector
{
public:
    struct GarbageBuffer 
    {
        VkBuffer buffer;
        VkDeviceMemory memory;
    };

    void pushBuffer(VkBuffer buffer, VkDeviceMemory memory, uint32_t inFlightFrameIndex);

    void cleanupFrame(VkDevice device, uint32_t inFlightFrameIndex);    
    void cleanup(VkDevice device);

private:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2; 

    std::vector<GarbageBuffer> buffersToDelete[MAX_FRAMES_IN_FLIGHT];
};