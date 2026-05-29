#include <renderer/renderer.hpp>
#include <core/swapchain.hpp>
#include <core/pipeline.hpp>
#include <core/device.hpp>
#include <core/buffer.hpp>
#include <stdexcept>

void Renderer::init(Device& device, VkSurfaceKHR surface, SwapChain* swapchain)
{
    this->device = device.getDevice();
    this->swapchain = swapchain;

    createQueues(device, surface);
    createSyncObjects();
    createRenderPass();
    createCommandPool(device, surface);
    createCommandBuffers();
}

void Renderer::cleanup(VkDevice device)
{
    vkDeviceWaitIdle(device);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
    {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
}

void Renderer::presentFrame(const Pipeline& pipeline, const Buffer& vertBuffer, const std::vector<Vertex>& vertices)
{
    // and here, we need to wait for fence
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // after wait, we need to reset fence
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    // let's get image index
    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(device, swapchain->swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        
    // and now, we can record commands in buffer
    // but, we need to reset whole buffer
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex, pipeline, vertBuffer, vertices);

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

const std::vector<VkCommandBuffer>& Renderer::getCommandBuffers() const
{
    return commandBuffers;
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

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;  
        
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

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

void Renderer::recordCommandBuffer(VkCommandBuffer buffer, uint32_t imageIndex, const Pipeline& pipeline, const Buffer& vertBuffer, const std::vector<Vertex>& vertices)
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
    VkClearValue clearColor = {{{0.f, 0.f, 0.f, 1.f}}}; 
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    // let's begin rendering something
    vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.graphicsPipeline);

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

    VkBuffer vertexBuffers[] = {vertBuffer.buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffers, offsets);

    // now, we are ready to draw
    vkCmdDraw(buffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);

    // and finish
    vkCmdEndRenderPass(buffer);

    if (vkEndCommandBuffer(buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }
}