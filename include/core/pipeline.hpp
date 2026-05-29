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

    void create(SwapChain& swapchain, VkDevice device, VkRenderPass renderPass, const std::string& vertPath, const std::string& fragPath);
    void cleanup(VkDevice device);

private:
    void createDescriptorSetLayout(VkDevice device);
    void createPipeline(SwapChain& swapchain, VkDevice device, VkRenderPass renderPass, const std::string& vertPath, const std::string& fragPath);
};