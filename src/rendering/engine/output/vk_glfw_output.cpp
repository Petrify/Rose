#include "vk_output.hpp"
#include "ve_view.hpp"

#include <algorithm>
#include <stdexcept>

const int MAX_FRAMES_IN_FLIGHT = 2;

VkGlfwOutput::VkGlfwOutput(GLFWwindow* window, VulkanEngine& engine) : window(window), VkOutput(engine) {
    glfwCreateWindowSurface(engine.vkInstance, window, nullptr, &surface);
    registerRequirements();
    if (engine.isInit()) {
        engine.requestPostInitialization(this);
    } else {
        init();
    }
}

void VkGlfwOutput::destroy() {
    if(isInit) {

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(engine.device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(engine.device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(engine.device, inFlightFences[i], nullptr);
        }

        for (auto framebuffer : framebuffers) {
            vkDestroyFramebuffer(engine.device, framebuffer, nullptr);
        }
        
        destroyView(view);

        vkDestroySwapchainKHR(engine.device, swapChain, nullptr);

        for(auto imageView : swapChainImageViews)
        {
            vkDestroyImageView(engine.device, imageView, nullptr);
        }
    }
    vkDestroySurfaceKHR(engine.vkInstance, surface, nullptr);
}

VkGlfwOutput::~VkGlfwOutput() {

}

const std::set<std::string> VkGlfwOutput::getRequiredInstanceExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::set<std::string> extensions;
    for (size_t i = 0; i < glfwExtensionCount; i++)
    {
        extensions.insert((std::string) glfwExtensions[i]);
    }
    return extensions;
}

void VkGlfwOutput::registerRequirements() {
    engine.requestDeviceExtensions(getRequiredDeviceExtensions());
    engine.requestDeviceRequirement([&] (VkPhysicalDevice device) -> bool {return this->checkDeviceRequirements(device);});
    engine.requestQueue([&] (const VkQueueFamilyProperties& prop, uint32_t idx, VkPhysicalDevice device) -> bool {return isGraphicsFamily(prop);}, &graphicsQueueFamily, &graphicsQueue);
    engine.requestQueue([&] (const VkQueueFamilyProperties& prop, uint32_t idx, VkPhysicalDevice device) -> bool {return isPresentFamily(prop, idx, device);}, &presentQueueFamily, &presentQueue);
}

void VkGlfwOutput::init() {
    createSwapChain();
    createImageViews();
    newView();
    createFramebuffers();
    createCommandBuffers();
    createSyncObjects();
    
    isInit = true;
}

void VkGlfwOutput::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = imageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(engine.device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void VkGlfwOutput::createSwapChain()
{
    swapChainSupport = querySwapChainSupport(engine.physicalDevice, surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    updateExtent();

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    
    uint32_t queueIndices[] = {graphicsQueueFamily, presentQueueFamily};

    if (graphicsQueueFamily != presentQueueFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(engine.device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(engine.device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(engine.device, swapChain, &imageCount, swapChainImages.data());

    imageFormat = surfaceFormat.format;
}

bool VkGlfwOutput::checkDeviceRequirements(VkPhysicalDevice device) const 
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) return false; 

    return true;
}

const std::set<std::string> VkGlfwOutput::getRequiredDeviceExtensions() const 
{  
    return requiredDeviceExtensions; 
}

void VkGlfwOutput::createRenderPass(View& view)
{
   VkAttachmentDescription colorAttachment{};
    colorAttachment.format = imageFormat;
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

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(engine.device, &renderPassInfo, nullptr, &view.renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    } 
}

void VkGlfwOutput::createPipelineBuilder(View& view) {
    view.graphicsPipelineBuilder = PipelineBuilder();
    view.graphicsPipelineBuilder.initBasic();
}

void VkGlfwOutput::updateExtent() {
    auto& capabilities = swapChainSupport.capabilities;

    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        extent = capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        extent = actualExtent;
    }
}

void VkGlfwOutput::createGraphicsPipeline(View& view) {

    VertexInputDescription description = Vertex::getVertexDescription();

    view.graphicsPipelineBuilder.vertexInputInfo.pVertexBindingDescriptions = description.bindings.data();
    view.graphicsPipelineBuilder.vertexInputInfo.vertexBindingDescriptionCount = (uint32_t) description.bindings.size();

    view.graphicsPipelineBuilder.vertexInputInfo.pVertexAttributeDescriptions = description.attributes.data();
    view.graphicsPipelineBuilder.vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t) description.attributes.size();

    VkShaderModule vertexShader = loadCompiledShader("../shaders/basic_mesh.vert.spv", engine.device);
    VkShaderModule fragmentShader = loadCompiledShader("../shaders/triangle.frag.spv", engine.device);

    view.graphicsPipelineBuilder.shaderStages.clear();
    view.graphicsPipelineBuilder.shaderStages.push_back(shaderStageInfo(vertexShader, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT));
    view.graphicsPipelineBuilder.shaderStages.push_back(shaderStageInfo(fragmentShader, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT));

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pNext = nullptr;
    pipelineLayoutInfo.setLayoutCount = 0;            // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(engine.device, &pipelineLayoutInfo, nullptr, &view.graphicsPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    view.graphicsPipelineBuilder.viewport.height = (float) extent.height;
    view.graphicsPipelineBuilder.viewport.width = (float) extent.width;

    view.graphicsPipelineBuilder.scissor.extent = extent;

    view.graphicsPipeline = view.graphicsPipelineBuilder.buildPipeline(engine.device, view.renderPass, view.graphicsPipelineLayout);

    vkDestroyShaderModule(engine.device, vertexShader, nullptr);
    vkDestroyShaderModule(engine.device, fragmentShader, nullptr);
}

void VkGlfwOutput::createFramebuffers() {
    framebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = view.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(engine.device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void VkGlfwOutput::aquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex){

}

void VkGlfwOutput::createCommandBuffers() {
    commandBuffers.resize(framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = engine.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(engine.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = view.renderPass;
        renderPassInfo.framebuffer = framebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent;

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, view.graphicsPipeline);

        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &engine.meshes[0].vertexBuffer.buffer, offsets);

        vkCmdDraw(commandBuffers[i], (uint32_t) engine.meshes[0].vertices.size(), 1, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

void VkGlfwOutput::draw() {
    vkWaitForFences(engine.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(engine.device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(engine.device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(engine.device, 1, &inFlightFences[currentFrame]);
    
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr; // Optional

    vkQueuePresentKHR(presentQueue, &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool VkGlfwOutput::isPresentFamily(const VkQueueFamilyProperties& prop, uint32_t idx, VkPhysicalDevice device) {
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, idx, surface, &presentSupport);
    return presentSupport;
}

View* VkGlfwOutput::newView() {
    view = View();
    createRenderPass(view);
    createPipelineBuilder(view);
    createGraphicsPipeline(view);
    return &view;
}

void VkGlfwOutput::destroyView(View& v) {
    vkDestroyPipeline(engine.device, v.graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(engine.device, v.graphicsPipelineLayout, nullptr);
    vkDestroyRenderPass(engine.device, v.renderPass, nullptr);
}

void VkGlfwOutput::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(engine.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(engine.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(engine.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

