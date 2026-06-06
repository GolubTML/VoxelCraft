#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

class Device;
class SwapChain;
class Renderer;

struct DebugInfo
{
    glm::vec3 playerPos;
    int chunksInMemory;
    int renderedChunks;
    int fps;
    int worldSeed;
    float deltaTime;
};

class DebugWindow
{
public:
    void initImGuiWindow(GLFWwindow* window, VkInstance instance, 
        const Device& device, const SwapChain& swapchain, const Renderer& renderer);
    void cleanup(const Device& device);

    void presentWindow(VkCommandBuffer buffer, DebugInfo& info);
private:
    VkDescriptorPool imGuiDescriptionPool;
    std::vector<float> frameTimeHistory;

    void createOwnDescriptionPool(const Device& device);

    void startFrame();
    // and here, should be something, that's takes info about everything in program
    void endFrame(VkCommandBuffer buffer);
};