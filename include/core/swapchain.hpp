#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

class Device;

class SwapChain
{
public:
    // here we have everething releted to swap chain.
    // also, we need method for it too
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    // Z buffer
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    const VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    void create(Device& device, VkSurfaceKHR surface, GLFWwindow* window);
    void cleanup(VkDevice device);
    
    // we cannot put this method in create, because we will have crush
    // so, let's use it independently
    void createFramebuffers(VkDevice device, VkRenderPass renderPass);

private:
    void createSwapchain(Device& device, VkSurfaceKHR surface, GLFWwindow* window);
    void createImageViews(VkDevice device);
    void createDepthResources(Device& device);

    // helpers
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
};