#include <renderer/renderer.hpp>
#include <core/swapchain.hpp>
#include <core/pipeline.hpp>
#include <core/device.hpp>
#include <core/camera.hpp>
#include <core/utils.hpp>
#include <renderer/types.hpp>
#include <game/chunk.hpp>
#include <game/world.hpp>
#include <renderer/texture.hpp>
#include <core/frustum.hpp>
#include <stdexcept>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstring>
#include <iostream>
#include <array>

void Renderer::init(Device& device, VkSurfaceKHR surface, SwapChain* swapchain)
{
    this->device = device.getDevice();
    this->swapchain = swapchain;

    createQueues(device, surface);
    createSyncObjects();
    createRenderPass();
    createCommandPool(device, surface);
    createCommandBuffers();
    createUniformBuffers(device);
    createDescriptorPool();
}

void Renderer::cleanup(VkDevice device)
{
    vkDeviceWaitIdle(device);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
    {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);

        uniformBuffers[i].cleanup(device);
    }

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
}

void Renderer::presentFrame(const Pipeline& pipeline, const Camera& camera, const Frustum& fCam, const World& world)
{
    // and here, we need to wait for fence
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // after wait, we need to reset fence
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    updateUniformBuffer(camera);

    // let's get image index
    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(device, swapchain->swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        
    // and now, we can record commands in buffer
    // but, we need to reset whole buffer
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex, fCam, pipeline, world);

    // let's submit it
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;


    // and submit all of this
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) // потенциальная ошибка тут
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapchain->swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr;

    // and present frame
    vkQueuePresentKHR(presentQueue, &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

VkRenderPass Renderer::getRenderPass() const
{
    return renderPass;
}

VkQueue Renderer::getGraphicsQueue() const
{
    return graphicsQueue;
}

VkCommandPool Renderer::getCommandPool() const
{
    return commandPool;
}

const std::vector<VkCommandBuffer>& Renderer::getCommandBuffers() const
{
    return commandBuffers;
}

void Renderer::createDescriptorPool()
{
    // firstly, we need pool size
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    // and now, create info as usual
    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    createInfo.pPoolSizes = poolSizes.data();
    createInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    // and now create
    if (vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create descriptor pool!");
    }
}

void Renderer::createDescriptorSet(const Pipeline& pipeline, const Texture2D& texture)
{
    // creating layouts
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, pipeline.descriptorSetLayout);
    // allocate info now
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    // and allocate it
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot allocate description set!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = texture.getImageView();
        imageInfo.sampler = texture.getSampler();

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1; 
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;


        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void Renderer::createUniformBuffers(Device& cDevice)
{
    // firstly, buffer size
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    // resize all vectors
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        uniformBuffers[i].create(cDevice.getPhysicalDevice(), device, 
            bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, nullptr);
            
        
        // and map to memory
        VkResult res = vkMapMemory(device,
            uniformBuffers[i].memory,
            0, bufferSize, 0,
            &uniformBuffersMapped[i]);

        if (res != VK_SUCCESS)
        {
            throw std::runtime_error("vkMapMemory failed!");
        }
    }
}   

void Renderer::updateUniformBuffer(const Camera& camera)
{
    // and here, basic UBO object will created
    UniformBufferObject ubo{};
    // view
    ubo.view = camera.getCameraView();
    // and projection
    ubo.proj = camera.getCameraProjection();

    // and we need to invert positions, because in vulkan Y is upside down
    ubo.proj[1][1] *= -1;

    // and now, we can copy data to uniform buffer
    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}


void Renderer::createQueues(Device& device, VkSurfaceKHR surface)
{
    // idk, is this good idea or not
    QueueFamilyIndices indices = device.getIndices();

    vkGetDeviceQueue(device.getDevice(), indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device.getDevice(), indices.presentFamily.value(), 0, &presentQueue);
}

void Renderer::createSyncObjects()
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    // for semaphores
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // and for fences
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // and create all 3 at once
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS 
        || vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS
        || vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Cannot create semaphores!");
        }
    }
}

void Renderer::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchain->swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // and now, we need to add depth description
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; 
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; 
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // and reference too
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;  
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create render pass!");
    }
}

void Renderer::createCommandPool(Device& device, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices = device.getIndices();

    VkCommandPoolCreateInfo commandPoolInfo{};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.queueFamilyIndex = indices.graphicsFamily.value();
    // this all for command pool, ebat

    if (vkCreateCommandPool(device.getDevice(), &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create command pool!");
    }
}

void Renderer::createCommandBuffers()
{
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    // let's create command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    // and so, creating
    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create command buffer!");
    }
}

void Renderer::recordCommandBuffer(VkCommandBuffer buffer, uint32_t imageIndex, const Frustum& fCam, const Pipeline& pipeline, const World& world)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr; // this is only for secondary command buffers

    if (vkBeginCommandBuffer(buffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to begin recording commands!");
    }

    // actual rendering?
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.framebuffer = swapchain->swapChainFramebuffers[imageIndex];
    renderPassInfo.renderPass = renderPass;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchain->swapChainExtent;

    // clear color. Yes, it actually rendering
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.f, 0.f, 0.f, 1.f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    // let's begin rendering something
    vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.graphicsPipeline);

    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

    // let's get view port
    VkViewport viewport{};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = static_cast<uint32_t>(swapchain->swapChainExtent.width);
    viewport.height = static_cast<uint32_t>(swapchain->swapChainExtent.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    // and here, we set view port
    vkCmdSetViewport(buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain->swapChainExtent;
    vkCmdSetScissor(buffer, 0, 1, &scissor);

    /*// vertex buffer
    VkBuffer vertexBuffers[] = {mesh.vertexBuffer.buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffers, offsets);
    // and index
    vkCmdBindIndexBuffer(buffer, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);*/

    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

    {
        // we added new thread, so, we should also use mutex, for sync

        std::lock_guard<std::mutex> lock(world.getChunkMutex());
        for (auto& [pos, chunk] : world.getChunks())
        {
            if (chunk->mesh.indexCount == 0) continue;

            glm::vec3 minBound(pos.x * 16, 0, pos.z * 16);
            glm::vec3 maxBound((pos.x + 1) * 16, 256, (pos.z + 1) * 16);

            if (!fCam.isBoxVisible(minBound, maxBound))
                continue;

            glm::mat4 modelMatrix = chunk->modelMatrix;
            // and pushing constant to GPU
            vkCmdPushConstants(
                buffer,
                pipeline.pipelineLayout,      
                VK_SHADER_STAGE_VERTEX_BIT, 
                0,                          
                sizeof(glm::mat4),          
                &modelMatrix                
            );

            VkBuffer vertexBuffers[] = { chunk->mesh.vertexBuffer.buffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(buffer, chunk->mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

            // and draw it
            vkCmdDrawIndexed(buffer, chunk->mesh.indexCount, 1, 0, 0, 0);
        }
    }
    

    // and finish
    vkCmdEndRenderPass(buffer);

    if (vkEndCommandBuffer(buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }
}