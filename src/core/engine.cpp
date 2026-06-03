#include <core/engine.hpp>
#include <stdexcept>
#include <iostream>


const uint32_t WINDOW_WIDTH = 1200;
const uint32_t WINDOW_HEIGHT = 900;

static void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    auto* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));

    if (camera)
        camera->mouse_callback(window, xpos, ypos);
}

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

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Engine::initVulkan()
{
    createInstance();
    Debug::setupDebugMessenger(instance, &debugMessenger);
    createSurface();
    device.init(instance, surface);
    swapchain.create(device, surface, window);
    mainCamera = Camera(glm::vec3(0.f, 65.f, 2.f), 60.f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);

    glfwSetWindowUserPointer(window, &mainCamera);
    glfwSetCursorPosCallback(window, mouse_callback);

    renderer.init(device, surface, &swapchain);

    pipeline.create(swapchain, device.getDevice(), renderer.getRenderPass(), "shaders/vert.spv", "shaders/frag.spv"); 

    testTexture.create(device, renderer, "assets/textures/blocks/block_atlas.png");

    renderer.createDescriptorSet(pipeline, testTexture);
    swapchain.createFramebuffers(device.getDevice(), renderer.getRenderPass());
    
    world = std::make_unique<World>(132416);
    world->initWorldThread(device);
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
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        int fps = (int)(1.f / deltaTime);
        std::string title = "VoxelCraft: " + std::to_string(fps);

        glfwSetWindowTitle(window, title.c_str());
        
        glfwPollEvents();
    
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        mainCamera.move(window, deltaTime);

        world->updatePlayerPos(mainCamera.pos);

        renderer.presentFrame(pipeline, mainCamera, *world);
    }

    vkDeviceWaitIdle(device.getDevice());
}

void Engine::cleanup() 
{ 
    Debug::destroyDebugMessenger(instance, debugMessenger);

    testTexture.cleanup(device);

    renderer.cleanup(device.getDevice());
    world->cleanup(device.getDevice());
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