#pragma once

#include <vulkan/vulkan.h>
#include <string>

class Device;
class Renderer;

class Texture2D
{
public:
    Texture2D() = default;
    ~Texture2D() = default;

    void create(Device& device, Renderer& renderer, std::string path);
    void cleanup(Device& device);

    VkImageView getImageView() const;
    VkSampler getSampler() const;

private:
    uint32_t mipLevels = 1;

    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;

    void createImage(Device& device, uint32_t width, uint32_t height, 
        VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, 
        VkMemoryPropertyFlags properties);
    
    void createTextureImageView(VkDevice device, VkFormat format, uint32_t mipLevels);
    void createSampler(Device& device);

    void transitionImageLayout(Device& device, Renderer& renderer, 
        VkImage image, VkFormat format, 
        VkImageLayout oldLayout, VkImageLayout newLayout,
        uint32_t mipLevels);
    void copyBufferToImage(Device& device, Renderer& renderer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void generateMipmaps(Device& device, Renderer& renderer, VkImage image,  VkFormat imageFormat,
        int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
};