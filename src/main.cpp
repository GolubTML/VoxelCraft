#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <cstring>
#include <map>
#include <optional>
#include <set>

#include <cstdint>
#include <algorithm>
#include <limits>
#include <fstream>

#include <glm/glm.hpp>
#include <array>

#include <renderer/mesh.hpp>
#include <core/swapchain.hpp>
#include <core/device.hpp>
#include <core/shader.hpp>
#include <core/buffer.hpp>
#include <core/pipeline.hpp>
#include <renderer/renderer.hpp>
#include <core/debugger.hpp>

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.f}, {1.f, 0.f, 0.f}},
    {{ 0.5f, -0.5f, 0.f}, {0.f, 1.f, 0.f}},
    {{ 0.5f,  0.5f, 0.f}, {0.f, 0.f, 1.f}},

    {{ 0.5f,  0.5f, 0.f}, {0.f, 0.f, 1.f}},
    {{-0.5f,  0.5f, 0.f}, {1.f, 1.f, 0.f}},
    {{-0.5f, -0.5f, 0.f}, {1.f, 0.f, 0.f}},
};

class Engine
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanUp();
    }

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

    void initWindow() 
    { 
        glfwInit();
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "VoxelCraft", nullptr, nullptr);
    }

    void initVulkan() 
    {
        createInstance();
        Debug::setupDebugMessenger(instance, &debugMessenger);
        createSurface();
        device.init(instance, surface);
        swapchain.create(device, surface, window);
        renderer.init(device, surface, &swapchain);
        pipeline.create(swapchain, device.getDevice(), renderer.getRenderPass(), "shaders/vert.spv", "shaders/frag.spv"); 
        swapchain.createFramebuffers(device.getDevice(), renderer.getRenderPass());

        VkDeviceSize bufferSize =sizeof(Vertex) * vertices.size();

        vertBuffer.create(device.getPhysicalDevice(), device.getDevice(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices.data());
    }

    void createSurface()
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Cannot create window surface!");
        }
    }

    void mainLoop() 
    { 
        while (!glfwWindowShouldClose(window)) 
        {
            glfwPollEvents();
            renderer.presentFrame(pipeline, vertBuffer, vertices);
        }

        vkDeviceWaitIdle(device.getDevice());
    }

    void cleanUp() 
    { 
        Debug::destroyDebugMessenger(instance, debugMessenger);

        renderer.cleanup(device.getDevice());
        vertBuffer.cleanup(device.getDevice());
        swapchain.cleanup(device.getDevice());
        pipeline.cleanup(device.getDevice());

        device.cleanup();
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        
        glfwTerminate();
    }

    void createInstance()
    {
        // let's use validation layer here
        if (Debug::enableValidationLayers && !Debug::checkValidationLayerSupport())
        {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        // creating info for application
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "VoxelCraft";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // creating instance create info
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo; // here, just cast appInfo

        // get all global extensions
        auto extensions = getRequiredExtentions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size()); // here, how many extensions we should enable
        createInfo.ppEnabledExtensionNames = extensions.data(); // and here, names of all extensions we are enable

        // let's create debug messenger info
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

        // now, let's use out validation layer
        if (Debug::enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(Debug::validationLayers.size());
            createInfo.ppEnabledLayerNames = Debug::validationLayers.data();

            Debug::populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        // and here, we are create instance
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw std::runtime_error("Cannot create instance!");
        }
    }

    std::vector<const char*> getRequiredExtentions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (Debug::enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }
};

int main()
{
    Engine app;

    try
    {
        app.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}