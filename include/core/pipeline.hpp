#pragma once

#include <vulkan/vulkan.h>
#include <core/shader.hpp>

class SwapChain;

class Pipeline
{
public:
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;

    VkPipeline graphicsPipeline;
    VkPipeline wireframePipeline;

    void create(VkDevice device);
    void cleanup(VkDevice device);

    VkPipeline createPipeline(SwapChain& swapchain, 
        VkDevice device, VkRenderPass renderPass, 
        const std::string& vertPath, const std::string& fragPath,
        VkPrimitiveTopology topology, VkPolygonMode polygonMode);

private:
    void createDescriptorSetLayout(VkDevice device);
};