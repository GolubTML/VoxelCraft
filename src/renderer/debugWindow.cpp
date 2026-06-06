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

    frameTimeHistory.resize(120, 0.f);
}

void DebugWindow::cleanup(const Device& device)
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(device.getDevice(), imGuiDescriptionPool, nullptr);
    frameTimeHistory.clear();
}

void DebugWindow::presentWindow(VkCommandBuffer buffer, DebugInfo& info)
{
    startFrame();

    static float smoothedFps = 60.0f; 
    if (info.fps > 0) 
    {
        smoothedFps = (smoothedFps * 0.95f) + (static_cast<float>(info.fps) * 0.05f);
    }

    size_t targetSize = static_cast<size_t>(smoothedFps * 1.5f);

    if (targetSize < 100)  targetSize = 100;
    if (targetSize > 1000) targetSize = 1000;

    if (frameTimeHistory.size() < targetSize) 
    {
        frameTimeHistory.insert(frameTimeHistory.end(), targetSize - frameTimeHistory.size(), 0.f);
    } 
    else if (frameTimeHistory.size() > targetSize) 
    {
        frameTimeHistory.erase(frameTimeHistory.begin(), frameTimeHistory.begin() + (frameTimeHistory.size() - targetSize));
    }

    frameTimeHistory.erase(frameTimeHistory.begin());
    frameTimeHistory.push_back(info.deltaTime * 1000.f);

    ImGui::Begin("Performance");
    ImGui::Separator();

    ImGui::Text("FPS count: %i", info.fps);
    ImGui::Text("Frame time (ms): %.2f", (info.deltaTime * 1000.f));

    ImVec2 graphSize = ImVec2(0, 60);

    ImGui::PlotLines("##frametime_graph", frameTimeHistory.data(), 
        static_cast<int>(frameTimeHistory.size()), 0, 
        "Frame Time (ms)", 0.0f, FLT_MAX,
        graphSize);

    ImGui::Separator();

    ImGui::Text("Player position: x:%.2f, y:%.2f, z:%.2f", info.playerPos.x, info.playerPos.y, info.playerPos.z);
    
    ImGui::Separator();
        
    ImGui::Text("Current chunks in memory: %i", info.chunksInMemory);
    ImGui::Text("Chunks rendered: %i", info.renderedChunks);
    ImGui::Text("World seed: %i", info.worldSeed);

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