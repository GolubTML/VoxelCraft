#pragma once

#include <optional>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

struct QueueFamilyIndices 
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class Device
{
public:
    void init(VkInstance instance, VkSurfaceKHR surface);
    void cleanup();

    VkDevice getDevice() const;
    VkPhysicalDevice getPhysicalDevice() const;
    QueueFamilyIndices getIndices() const;

private:
    std::vector<const char*> deviceExtensions;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    QueueFamilyIndices indices;

    void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surfac);
    void createLogicalDevice(VkSurfaceKHR surface);
    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
    bool chechDeviceExtensionSuppot(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
};