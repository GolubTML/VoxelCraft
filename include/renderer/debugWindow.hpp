#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

class Device;
class SwapChain;
class Renderer;

class DebugWindow
{
public:
    void initImGuiWindow(GLFWwindow* window, VkInstance instance, 
        const Device& device, const SwapChain& swapchain, const Renderer& renderer);
    void cleanup(const Device& device);

    void presentWindow(VkCommandBuffer buffer);
private:
    VkDescriptorPool imGuiDescriptionPool;

    void createOwnDescriptionPool(const Device& device);

    void startFrame();
    // and here, should be something, that's takes info about everything in program
    void endFrame(VkCommandBuffer buffer);
};