#pragma once

#include <vulkan/vulkan.h>
#include <core/shader.hpp>

class SwapChain;

class Pipeline
{
public:
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    void create(SwapChain& swapchain, VkDevice device, VkRenderPass renderPass, const std::string& vertPath, const std::string& fragPath);
    void cleanup(VkDevice device);

};