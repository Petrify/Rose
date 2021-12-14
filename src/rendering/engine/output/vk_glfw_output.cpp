#include "vk_output.hpp"

#include <algorithm>
#include <stdexcept>

VkGlfwOutput::VkGlfwOutput(GLFWwindow* window) : window(window) {}

VkGlfwOutput::~VkGlfwOutput() {}

void VkGlfwOutput::bind(VkInstance instance) {
    this->instance = instance;
    glfwCreateWindowSurface(instance, window, nullptr, &surface);
}

void VkGlfwOutput::unbind() {
    if(isInit) {
        for (auto framebuffer : framebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
        vkDestroySwapchainKHR(device, swapChain, nullptr);

        for(auto imageView : swapChainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }

        device = VK_NULL_HANDLE;
        physicalDevice = VK_NULL_HANDLE;
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    this->instance = VK_NULL_HANDLE;
}

void VkGlfwOutput::init(VkDevice device, VkPhysicalDevice physicalDevice, std::vector<uint32_t> queueIndicies) {
    this->device = device;
    this->physicalDevice = physicalDevice;

    queueFamilyIndicies = getRequiredQueueFamilies(physicalDevice);
    queues.resize(queueFamilyIndicies.size());

    for (size_t i = 0; i < queueFamilyIndicies.size(); i++)
    {
        vkGetDeviceQueue(device, queueFamilyIndicies[i], queueIndicies[i], &queues[i]);
    }

    createSwapChain();
    createImageViews();
    createRenderPass();
    createPipelineBuilders();
    createGraphicsPipeline();
    createFramebuffers();
    

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

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void VkGlfwOutput::createSwapChain()
{
    swapChainSupport = querySwapChainSupport(physicalDevice, surface);

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

    
    uint32_t queueIndices[] = {queueFamilyIndicies[QueueRef::graphicsIdx], queueFamilyIndicies[QueueRef::presentIdx]};

    if (queueFamilyIndicies[QueueRef::graphicsIdx] != queueFamilyIndicies[QueueRef::presentIdx])
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

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    imageFormat = surfaceFormat.format;
}

bool VkGlfwOutput::checkDeviceRequirements(VkPhysicalDevice device) const 
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) return false;

    auto optIndicies = _getRequiredQueueFamilies(physicalDevice);
    for (size_t i = 0; i < QueueRef::_count; i++)
    {
        if (!optIndicies[i].has_value()) return false;
    } 

    return true;
}

const std::vector<std::string> VkGlfwOutput::getRequiredExtensions() const 
{
    auto extensions = std::set<std::string>(requiredExtensions); 
    
    std::vector<std::string> out;

    out.resize(extensions.size());
    std::copy(extensions.begin(), extensions.end(), out.begin());
    
    return out; 
}

std::vector<uint32_t> VkGlfwOutput::getRequiredQueueFamilies(VkPhysicalDevice device) const
{
    std::vector<uint32_t> indicies;
    indicies.resize(QueueRef::_count);
    auto optIndicies = _getRequiredQueueFamilies(device);
    for (size_t i = 0; i < QueueRef::_count; i++)
    {
        if (!optIndicies[i].has_value()) std::runtime_error("Device does not support required queues");
        indicies[i] = optIndicies[i].value();
    } 

    return indicies;
}

std::vector<std::optional<uint32_t>> VkGlfwOutput::_getRequiredQueueFamilies(VkPhysicalDevice device) const
{
    std::vector<std::optional<uint32_t>> indicies;
    indicies.resize(QueueRef::_count);

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) indicies[QueueRef::presentIdx] = i;

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) indicies[QueueRef::graphicsIdx] = i;

        i++;
    }

    return indicies;
}

void VkGlfwOutput::createRenderPass()
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

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

void VkGlfwOutput::createPipelineBuilders() {
    graphicsPipelineBuilder = PipelineBuilder();
    graphicsPipelineBuilder.initBasic();
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

void VkGlfwOutput::createGraphicsPipeline() {

    VkShaderModule vertexShader = loadCompiledShader("../shaders/triangle.vert.spv", device);
    VkShaderModule fragmentShader = loadCompiledShader("../shaders/triangle.frag.spv", device);

    graphicsPipelineBuilder.shaderStages.push_back(shaderStageInfo(vertexShader, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT));
    graphicsPipelineBuilder.shaderStages.push_back(shaderStageInfo(fragmentShader, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT));

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pNext = nullptr;
    pipelineLayoutInfo.setLayoutCount = 0;            // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &graphicsPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    graphicsPipelineBuilder.viewport.height = (float) extent.height;
    graphicsPipelineBuilder.viewport.width = (float) extent.width;

    graphicsPipelineBuilder.scissor.extent = extent;

    graphicsPipeline = graphicsPipelineBuilder.buildPipeline(device, renderPass, graphicsPipelineLayout);

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragmentShader, nullptr);
}

void VkGlfwOutput::createFramebuffers() {
    framebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
        }
}

void VkGlfwOutput::aquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex){

}

void VkGlfwOutput::draw() {
    
}