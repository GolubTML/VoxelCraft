#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

class Shader
{
public:
    Shader(const std::string& path, VkDevice device, VkShaderStageFlagBits shaderFlag);

    void load(const std::string& path, VkDevice device, VkShaderStageFlagBits shaderFlag);
    void cleanup(VkDevice device);

    VkPipelineShaderStageCreateInfo getStageInfo() const;

private:
    VkShaderModule module;
    VkPipelineShaderStageCreateInfo shaderStage{};

    // idk where put this method, so, it will be here
    std::vector<char> readFile(const std::string& path);

};