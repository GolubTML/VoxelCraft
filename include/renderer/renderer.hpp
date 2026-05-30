#pragma once

#include <vulkan/vulkan.h>
#include <renderer/mesh.hpp>
#include <core/buffer.hpp>
#include <vector>

const int MAX_FRAMES_IN_FLIGHT = 2;

class SwapChain;
class Pipeline;
class Device;
class Mesh;

class Renderer
{
public:
    void init(Device& device, VkSurfaceKHR surface, SwapChain* swapchain);
    void cleanup(VkDevice device);
    
    // BAD. Need new architecture for this
    void createDescriptorSet(const Pipeline& pipeline);

    void presentFrame(const Pipeline& pipeline, Mesh& mesh);

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

    std::vector<Buffer> uniformBuffers;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    
    uint32_t currentFrame = 0;

    // descriptors
    void createDescriptorPool();
    // buffers
    void createUniformBuffers(Device& cDevice);
    void updateUniformBuffer();
    // queues
    void createQueues(Device& device, VkSurfaceKHR surface);
    // render
    void createSyncObjects();
    void createRenderPass();
    void createCommandPool(Device& device, VkSurfaceKHR surface);
    void createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer buffer, uint32_t imageIndex, const Pipeline& pipeline, Mesh& mesh);
};