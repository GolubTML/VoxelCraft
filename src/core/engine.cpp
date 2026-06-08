#include <core/engine.hpp>
#include <stdexcept>
#include <iostream>
#include "lib/imgui/imgui.h"

unsigned int getRandomSeed() 
{
    // for test, it will be here
    using namespace std::chrono;

    return static_cast<unsigned int>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()
    );
}

static void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
        return; 

    auto* player = static_cast<Player*>(glfwGetWindowUserPointer(window));
    if (player)
        player->handleMouse(window, xpos, ypos);
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
    player = std::make_unique<Player>(glm::vec3(0.f, 64.f, 0.f), 8.f, 60.f, WINDOW_WIDTH, WINDOW_HEIGHT);

    glfwSetWindowUserPointer(window, player.get());
    glfwSetCursorPosCallback(window, mouse_callback);

    renderer.init(device, surface, &swapchain);

    pipeline.create(swapchain, device.getDevice(), renderer.getRenderPass(), "shaders/vert.spv", "shaders/frag.spv"); 

    testTexture.create(device, renderer, "assets/textures/blocks/block_atlas.png");

    renderer.createDescriptorSet(pipeline, testTexture);
    swapchain.createFramebuffers(device.getDevice(), renderer.getRenderPass());
    
    debugWindow.initImGuiWindow(window, instance, device, swapchain, renderer);

    world = std::make_unique<World>(getRandomSeed());
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
        
        glfwPollEvents();

        player->update(window, deltaTime);
        frustumCam.update(player->getPlayerCamera().getCameraProjection() * player->getPlayerCamera().getCameraView());

        uint32_t frameIndex = renderer.getCurrentFrame();
        gc.cleanupFrame(device.getDevice(), frameIndex);

        DebugInfo info{};
        info.playerPos = player->getPlayerCamera().pos;
        info.chunksInMemory = world->getChunks().size();
        info.renderedChunks = renderer.getAllRendererChunks();
        info.fps = (int)(1.f / deltaTime);
        info.worldSeed = world->getWorldSeed();
        info.deltaTime = deltaTime;

        world->updatePlayerPos(player->getPlayerPosition());
        world->uploadChunksToGpu(renderer, gc, frameIndex);
        
        renderer.presentFrame(pipeline, player->getPlayerCamera(), 
            frustumCam, debugWindow, 
            *world, info);

        input();
    }

    vkDeviceWaitIdle(device.getDevice());
}

void Engine::cleanup() 
{ 
    Debug::destroyDebugMessenger(instance, debugMessenger);

    testTexture.cleanup(device);
    gc.cleanup(device.getDevice());

    world->cleanup(device.getDevice());
    debugWindow.cleanup(device);
    renderer.cleanup(device.getDevice());
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

void Engine::input()
{
    static bool tabPressed = false;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !tabPressed)
    {
        int currentMode = glfwGetInputMode(window, GLFW_CURSOR);

        if (currentMode == GLFW_CURSOR_DISABLED)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            player->getPlayerCamera().firstMouse = true;
        }

        tabPressed = true; 
    }

    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE)
    {
        tabPressed = false;
    }

    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) 
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
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