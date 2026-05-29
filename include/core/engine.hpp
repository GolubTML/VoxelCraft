#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <renderer/mesh.hpp>
#include <core/swapchain.hpp>
#include <core/device.hpp>
#include <core/shader.hpp>
#include <core/buffer.hpp>
#include <core/pipeline.hpp>
#include <renderer/renderer.hpp>
#include <core/debugger.hpp>

class Engine
{
public:
    void run();

private:
    GLFWwindow* window = nullptr;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    // Devices
    Device device;
    // Window surface
    VkSurfaceKHR surface;

    SwapChain swapchain;
    // Pipeline
    Renderer renderer;
    Pipeline pipeline;
    // buffer
    Buffer vertBuffer;
    Buffer indexBuffer;

    void initWindow();
    void initVulkan();
    void createSurface(); // idk where put it
    void mainLoop();
    void cleanup();
    void createInstance();
    std::vector<const char*> getRequiredExtentions();
};