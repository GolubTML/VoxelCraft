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
#include <game/world.hpp>
#include <renderer/texture.hpp>
#include <core/frustum.hpp>
#include <renderer/debugWindow.hpp>
#include <core/garbageCollector.hpp>
#include <game/player.hpp>
#include <memory>

class Engine
{
public:
    static constexpr const uint32_t WINDOW_WIDTH = 1200;
    static constexpr const uint32_t WINDOW_HEIGHT = 900;

    void run();

private:
    GLFWwindow* window = nullptr;
    DebugWindow debugWindow;
    VkInstance instance;
    GarbageCollector gc;
    VkDebugUtilsMessengerEXT debugMessenger;
    // Devices
    Device device;
    // Window surface
    VkSurfaceKHR surface;
    // swapchain
    SwapChain swapchain;
    // camera
    std::unique_ptr<Player> player;
    Frustum frustumCam;
    // Pipeline
    Renderer renderer;
    Pipeline pipeline;
    
    Texture2D testTexture;

    std::unique_ptr<World> world;

    float lastTime = 0.f;

    void initWindow();
    void initVulkan();
    void createSurface(); // idk where put it
    void mainLoop();
    void cleanup();
    void createInstance();

    void input();

    std::vector<const char*> getRequiredExtentions();
};