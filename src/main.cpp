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
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
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
        pickPhysicalDevice();
        createLogicalDevice();
        swapchain.create(physicalDevice, device, surface, window);
        renderer.init(device, physicalDevice, surface, &swapchain);
        pipeline.create(swapchain, device, renderer.getRenderPass(), "shaders/vert.spv", "shaders/frag.spv"); 
        swapchain.createFramebuffers(device, renderer.getRenderPass());

        VkDeviceSize bufferSize =sizeof(Vertex) * vertices.size();

        vertBuffer.create(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices.data());
    }

    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
        
        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) 
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create logical device!");
        }
    }

    void pickPhysicalDevice()
    {
        // let's get all GPUs with Vulkan support
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0)
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices)
        {
            if (isDeviceSuitable(device))
            {
                physicalDevice = device; // we get GPU
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) 
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
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

        vkDeviceWaitIdle(device);
    }

    void cleanUp() 
    { 
        Debug::destroyDebugMessenger(instance, debugMessenger);

        renderer.cleanup(device);
        vertBuffer.cleanup(device);
        swapchain.cleanup(device);
        pipeline.cleanup(device);

        vkDestroyDevice(device, nullptr);
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

    bool isDeviceSuitable(VkPhysicalDevice device) 
    {
        QueueFamilyIndices indices = findQueueFamilies(device, surface);

        bool extensionSupported = chechDeviceExtensionSuppot(device);

        bool swapChainAdequate = false;
        if (extensionSupported)
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionSupported && swapChainAdequate;
    }

    bool chechDeviceExtensionSuppot(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) 
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
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