#include "vk_output.hpp"
#include "ve_view.hpp"

#include <glm/gtx/transform.hpp>

#include <algorithm>
#include <stdexcept>

const int MAX_FRAMES_IN_FLIGHT = 2;

VkGlfwOutput::VkGlfwOutput(GLFWwindow* window, VulkanEngine& engine) : window(window), VkOutput(engine) {
    glfwCreateWindowSurface(engine.vkInstance, window, nullptr, &surface);
    registerRequirements();
    if (!engine.isInit()) {
        engine.requestPostInitialization(this);   
    } else {
        init();
    }
}

void VkGlfwOutput::destroy() {
  
    if (isInit)
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(engine.device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(engine.device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(engine.device, inFlightFences[i], nullptr);
        }

        for (auto framebuffer : framebuffers) {
            vkDestroyFramebuffer(engine.device, framebuffer, nullptr);
        }
        
        destroyView(view);
        vkDestroyImageView(engine.device, depthImageView, nullptr);
        vmaDestroyImage(engine.allocator, depthImage._image, depthImage._allocation);
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
    initImgui();
    
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

    VkExtent3D depthImageExtent = {
        extent.width,
        extent.height,
        1
    };

    //hardcoding the depth format to 32 bit float
	depthFormat = VK_FORMAT_D32_SFLOAT;

	//the depth image will be an image with the format we selected and Depth Attachment usage flag
	VkImageCreateInfo dimg_info = {};

    dimg_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    dimg_info.pNext = nullptr;

    dimg_info.imageType = VK_IMAGE_TYPE_2D;

    dimg_info.format = depthFormat;
    dimg_info.extent = depthImageExtent;

    dimg_info.mipLevels = 1;
    dimg_info.arrayLayers = 1;
    dimg_info.samples = VK_SAMPLE_COUNT_1_BIT;
    dimg_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    dimg_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	//for the depth image, we want to allocate it from GPU local memory
	VmaAllocationCreateInfo dimg_allocinfo = {};
	dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(engine.allocator, &dimg_info, &dimg_allocinfo, &depthImage._image, &depthImage._allocation, nullptr);

	//build an image-view for the depth image to use for rendering
	VkImageViewCreateInfo dview_info = {};
    dview_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	dview_info.pNext = nullptr;

	dview_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	dview_info.image = depthImage._image;
	dview_info.format = depthFormat;
	dview_info.subresourceRange.baseMipLevel = 0;
	dview_info.subresourceRange.levelCount = 1;
	dview_info.subresourceRange.baseArrayLayer = 0;
	dview_info.subresourceRange.layerCount = 1;
	dview_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

	vkCreateImageView(engine.device, &dview_info, nullptr, &depthImageView);
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

    VkAttachmentDescription depth_attachment = {};
    // Depth attachment
    depth_attachment.flags = 0;
    depth_attachment.format = depthFormat;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref = {};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency depth_dependency = {};
    depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depth_dependency.dstSubpass = 0;
    depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depth_dependency.srcAccessMask = 0;
    depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency dependencies[2] = { dependency, depth_dependency };

    VkAttachmentDescription attachments[2] = { colorAttachment,depth_attachment };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;

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

    VkPushConstantRange range;
    range.offset = 0;
    range.size = sizeof(MeshPushConstants);
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pNext = nullptr;
    pipelineLayoutInfo.setLayoutCount = 0;            // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 1;    
    pipelineLayoutInfo.pPushConstantRanges = &range; 

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
            swapChainImageViews[i],
            depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = view.renderPass;
        framebufferInfo.attachmentCount = 2;
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

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; 
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VkClearValue depthClear;
	depthClear.depthStencil.depth = 1.f;
    VkClearValue clearValues[] = { view.clearColor, depthClear };

    VkCommandBuffer& cmd = commandBuffers[imageIndex];

    ImGui::Render();

    glm::mat4 camView = glm::translate(glm::mat4(1.0f), view.cameraPos);
    //camera projection
    glm::mat4 projection = glm::perspective(glm::radians(view.fov.x), 1700.f / 900.f, 0.1f, 200.0f);
    projection[1][1] *= -1;
    //model rotation
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(glm::radians(view.fov.y)) , glm::vec3(0, 1, 0));

    pushConstants.render_matrix = projection * camView * model;
    pushConstants.offset = glm::vec4(1);

    // ============== BEGIN COMMAND BUFFER ==============

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = view.renderPass;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;

    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, view.graphicsPipeline);

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &engine.meshes[0].vertexBuffer.buffer, offsets);

    vkCmdPushConstants(cmd, view.graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &pushConstants);

    vkCmdDraw(cmd, (uint32_t) engine.meshes[0].vertices.size(), 1, 0, 0);
    
    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

    // ============== END COMMAND BUFFER ==============

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

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

void VkGlfwOutput::initImgui() {

    // Imgui
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = engine.vkInstance;
    init_info.PhysicalDevice = engine.physicalDevice;
    init_info.Device = engine.device;
    init_info.QueueFamily = graphicsQueueFamily;
    init_info.Queue = graphicsQueue;
    init_info.PipelineCache = engine.pipelineCache;
    init_info.DescriptorPool = engine.descriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = (uint32_t) swapChainImages.size();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, view.renderPass);

     // Upload Fonts
    VkCommandBuffer commandBuffer;

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = engine.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(engine.device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
    
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check_vk_result(vkBeginCommandBuffer(commandBuffer, &begin_info));

    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

    VkSubmitInfo end_info = {};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &commandBuffer;
    check_vk_result(vkEndCommandBuffer(commandBuffer));
    check_vk_result(vkQueueSubmit(graphicsQueue, 1, &end_info, VK_NULL_HANDLE));


    check_vk_result(vkDeviceWaitIdle(engine.device));
    vkFreeCommandBuffers(engine.device, engine.commandPool, 1, &commandBuffer);
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    // Supply Imgui with a single empty frame so it does not crash
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::EndFrame();
}

void VkGlfwOutput::beginImguiFrame() {
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    #ifdef _DEBUG
        ImGui::ShowDemoWindow((bool*) true);    
    #endif // DEBUG
}

void VkGlfwOutput::endImguiFrame() {
    ImGui::EndFrame();
}