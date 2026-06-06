#include <renderer/debugWindow.hpp>
#include <core/device.hpp>
#include <core/swapchain.hpp>
#include <renderer/renderer.hpp>

#include "lib/imgui/imgui.h"
#include "lib/imgui/backends/imgui_impl_glfw.h"
#include "lib/imgui/backends/imgui_impl_vulkan.h"

#include <stdexcept>

void DebugWindow::initImGuiWindow(GLFWwindow* window, VkInstance instance, 
    const Device& device, const SwapChain& swapchain, const Renderer& renderer)
{
    createOwnDescriptionPool(device);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark(); 

    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = instance;          
    initInfo.PhysicalDevice = device.getPhysicalDevice(); 
    initInfo.Device = device.getDevice();              
    initInfo.QueueFamily = device.getIndices().graphicsFamily.value(); 
    initInfo.Queue = renderer.getGraphicsQueue();                     
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = this->imGuiDescriptionPool;      
    initInfo.Subpass = 0;                             
    initInfo.MinImageCount = 2;                        
    initInfo.ImageCount = swapchain.swapChainImages.size();    
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;      
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = nullptr;
    initInfo.RenderPass = renderer.getRenderPass();

    ImGui_ImplVulkan_Init(&initInfo);
}

void DebugWindow::cleanup(const Device& device)
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(device.getDevice(), imGuiDescriptionPool, nullptr);
}

void DebugWindow::presentWindow(VkCommandBuffer buffer)
{
    startFrame();
    
    ImGui::Begin("Hi!");

    endFrame(buffer);
}

void DebugWindow::createOwnDescriptionPool(const Device& device)
{
    VkDescriptorPoolSize poolSizes[] = 
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000 * ((sizeof(poolSizes) / sizeof(*(poolSizes))));
    poolInfo.poolSizeCount = (uint32_t)((sizeof(poolSizes) / sizeof(*(poolSizes))));
    poolInfo.pPoolSizes = poolSizes;

    if (vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &imGuiDescriptionPool) != VK_SUCCESS) 
    {
        throw std::runtime_error("Failed to create code descriptor pool for ImGui!");
    }
}

void DebugWindow::startFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void DebugWindow::endFrame(VkCommandBuffer buffer)
{
    ImGui::End();
    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buffer);
}