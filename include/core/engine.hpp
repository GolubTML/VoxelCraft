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
#include <renderer/mesh.hpp>
#include <core/camera.hpp>
#include <memory>

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
    // swapchain
    SwapChain swapchain;
    // camera
    Camera mainCamera;
    // Pipeline
    Renderer renderer;
    Pipeline pipeline;
    // mesh
    Mesh testMesh;

    float lastTime = 0.f;

    void initWindow();
    void initVulkan();
    void createSurface(); // idk where put it
    void mainLoop();
    void cleanup();
    void createInstance();
    std::vector<const char*> getRequiredExtentions();
};