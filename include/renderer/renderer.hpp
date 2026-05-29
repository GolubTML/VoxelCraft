#pragma once

#include <vulkan/vulkan.h>
#include <renderer/mesh.hpp>
#include <vector>

const int MAX_FRAMES_IN_FLIGHT = 2;

class SwapChain;
class Pipeline;
class Buffer;

class Renderer
{
public:
    void init(VkDevice device, VkPhysicalDevice pDevice, VkSurfaceKHR surface, SwapChain* swapchain);
    void cleanup(VkDevice device);

    void presentFrame(const Pipeline& pipeline, const Buffer& vertBuffer, const std::vector<Vertex>& vertices);

    VkRenderPass getRenderPass() const;
    const std::vector<VkCommandBuffer>& getCommandBuffers() const;

private:
    SwapChain* swapchain = nullptr;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkRenderPass renderPass;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    
    uint32_t currentFrame = 0;

    void createQueues(VkPhysicalDevice pDevice, VkSurfaceKHR surface);
    void createSyncObjects();
    void createRenderPass();
    void createCommandPool(VkPhysicalDevice pDevice, VkSurfaceKHR surface);
    void createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer buffer, uint32_t imageIndex, const Pipeline& pipeline, const Buffer& vertBuffer, const std::vector<Vertex>& vertices);
};