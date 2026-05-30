#include <core/engine.hpp>
#include <stdexcept>
#include <iostream>

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f,  0.5f}, {0.4f, 1.f, 0.1f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.4f, 1.f, 0.1f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.4f, 1.f, 0.1f}},
    {{-0.5f,  0.5f,  0.5f}, {0.4f, 1.f, 0.1f}},
    {{-0.5f, -0.5f, -0.5f}, {0.4f, 1.f, 0.1f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.4f, 1.f, 0.1f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.4f, 1.f, 0.1f}},
    {{-0.5f,  0.5f, -0.5f}, {0.4f, 1.f, 0.1f}} 
};

const std::vector<uint32_t> indices = {
    0, 1, 2, 2, 3, 0, 
    4, 5, 6, 6, 7, 4, 
    4, 0, 3, 3, 7, 4, 
    1, 5, 6, 6, 2, 1, 
    3, 2, 6, 6, 7, 3, 
    4, 5, 1, 1, 0, 4  
};

void Engine::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void Engine::initWindow()
{
    glfwInit();
        
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "VoxelCraft", nullptr, nullptr);
}

void Engine::initVulkan()
{
    createInstance();
    Debug::setupDebugMessenger(instance, &debugMessenger);
    createSurface();
    device.init(instance, surface);

    swapchain.create(device, surface, window);
    renderer.init(device, surface, &swapchain);

    pipeline.create(swapchain, device.getDevice(), renderer.getRenderPass(), "shaders/vert.spv", "shaders/frag.spv"); 
    
    renderer.createDescriptorSet(pipeline);
    
    swapchain.createFramebuffers(device.getDevice(), renderer.getRenderPass());

    testMesh.create(device, vertices, indices);
}

void Engine::createSurface()
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create window surface!");
    }
}

void Engine::mainLoop()
{
    while (!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();
        renderer.presentFrame(pipeline, testMesh);
    }

    vkDeviceWaitIdle(device.getDevice());
}

void Engine::cleanup() 
{ 
    Debug::destroyDebugMessenger(instance, debugMessenger);

    renderer.cleanup(device.getDevice());
    testMesh.cleanup(device.getDevice());
    swapchain.cleanup(device.getDevice());
    pipeline.cleanup(device.getDevice());

    device.cleanup();
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    
    glfwTerminate();
}

void Engine::createInstance()
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

std::vector<const char*> Engine::getRequiredExtentions()
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