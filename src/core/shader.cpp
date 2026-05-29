#include <core/shader.hpp>
#include <stdexcept>
#include <fstream>

Shader::Shader(const std::string& path, VkDevice device, VkShaderStageFlagBits shaderFlag)
{
    auto shaderCode = readFile(path);

    // creating createInfo 
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create shader module!");
    }

    // and here, also, we create stage info
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = shaderFlag;
    shaderStage.module = module;
    shaderStage.pName = "main"; 
}

void Shader::load(const std::string& path, VkDevice device, VkShaderStageFlagBits shaderFlag)
{
    auto shaderCode = readFile(path);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create shader module!");
    }

    // and here, also, we create stage info
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = shaderFlag;
    shaderStage.module = module;
    shaderStage.pName = "main"; 
}

void Shader::cleanup(VkDevice device)
{
    vkDestroyShaderModule(device, module, nullptr);
}

VkPipelineShaderStageCreateInfo Shader::getStageInfo() const
{
    return shaderStage;
}

std::vector<char> Shader::readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) 
    {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}