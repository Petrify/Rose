#include "vk_engine.hpp"

#include "vk_debug.hpp"
#include "vk_swapchain.hpp"
#include "ve_pipeline.hpp"

struct View {
    std::vector<VkFramebuffer> swapChainFramebuffers;
    // Rendering
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkCommandBuffer> commandBuffers;

    // Sync
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
};

VulkanEngine::VulkanEngine(bool useGlfw) : useGlfw(useGlfw)
{
    vkLogger = getLogger("VulkanEngine");
    createInstance();
    setupDebugMessenger();
}

VulkanEngine::~VulkanEngine()
{
    if (enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);
    }

    vkDestroyInstance(vkInstance, nullptr);
}

void VulkanEngine::init() {
    vkLogger->info("Initializing Vulkan Render Engine");

    compileDeviceExtensions();
    pickPhysicalDevice();
    createLogicalDevice();
    initOutputs();
    // input createRenderPass();
    // input createGraphicsPipeline();
    // input createFramebuffers();
    createCommandPool();
    createVertexBuffer(); // TODO:
    // output createCommandBuffers();
    // input / output createSyncObjects();
}

void VulkanEngine::shutdown() {

    vkDestroyCommandPool(device, commandPool, nullptr);

    //vkDestroyBuffer(device, vertexBuffer, nullptr);
    //vkFreeMemory(device, vertexBufferMemory, nullptr);

    for(auto output : outputs) {removeOutput(output);}

    vkDestroyDevice(device, nullptr);
}

void VulkanEngine::start() {

}

void VulkanEngine::stop() 
{
    vkDeviceWaitIdle(device);
}

void VulkanEngine::createInstance()
{
    if (enableValidationLayers && !checkValidationLayerSupport(validationLayers))
    {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine"; 
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    // Actually create Instance
    if (vkCreateInstance(&createInfo, nullptr, &vkInstance) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create instance!");
    }

    // Query Available Vk Extentions
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    vkLogger->debug("Available Vulkan extentions:");

    for (const auto &extension : availableExtensions)
    {
        vkLogger->debug(extension.extensionName);
    }
}

void VulkanEngine::setupDebugMessenger()
{
    if (!enableValidationLayers)
        return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(vkInstance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void VulkanEngine::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

    for (const auto &device : devices)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        vkLogger->debug("Checking device: {}", properties.deviceName);
        if (isDeviceSuitable(device))
        {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void VulkanEngine::compileDeviceExtensions() {
    

    std::set<std::string> extensions;

    for(const auto& ext : engineExtensions)
    {
        extensions.insert(ext);
    }

    
    for(const auto& output : outputs)
    {
        auto outputExtensions =  output->getRequiredExtensions();
        for(const auto& ext : outputExtensions)
        {
            extensions.insert(ext);
        }
    }

    deviceExtensions = {};
    _deviceExtensions = {};
    for(const auto& ext : extensions)
    {   
        deviceExtensions.push_back(std::string(ext));
        _deviceExtensions.push_back(deviceExtensions.back().c_str()); 
    }
}

void VulkanEngine::createLogicalDevice()
{
    queueIndicies = findQueueFamilies(physicalDevice);

    std::set<uint32_t> requestedQueueFamilies = { // Init with Engine Queues
        queueIndicies.graphicsFamily.value()
    };

    // Gather requested queues from Outputs
    for(auto output : outputs){
        for(auto familyIdx : output->getRequiredQueueFamilies(physicalDevice)){
            requestedQueueFamilies.insert(familyIdx);
        }
    }  

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : requestedQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    std::vector<const char*> extensions(_deviceExtensions.size());
    std::copy(_deviceExtensions.begin(), _deviceExtensions.end(), extensions.begin());
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Validation Layers are now instance Specific
    // Definition here is only for backwards compatibility
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    // Populate device queue handles
    vkGetDeviceQueue(device, queueIndicies.graphicsFamily.value(), 0, &graphicsQueue);
}

void VulkanEngine::initOutputs() {
    for(auto output : outputs)
    {
        auto requested = output->getRequiredQueueFamilies(physicalDevice);

        // which queue in the respective family to use (for now only one queue is created so always 0)
        std::vector<uint32_t> queueIndicies(requested.size(), 0);

        output->init(device, physicalDevice, queueIndicies);
    }   
}

void VulkanEngine::createCommandPool() 
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = 0; // Optional

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void VulkanEngine::createVertexBuffer() 
{

}

std::vector<const char *> VulkanEngine::getRequiredExtensions()
{
    std::vector<const char *> extensions(0);

    if(useGlfw) {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        for (std::uint32_t i{}; i < glfwExtensionCount; ++i)
        {
            extensions.push_back(glfwExtensions[i]);
        }
    }

    // Validation Layers
    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void VulkanEngine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

    vkQueueWaitIdle(graphicsQueue); // Bad but ok here for now 

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

bool VulkanEngine::isDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indicies = findQueueFamilies(device);

    if (!indicies.isComplete()) {
        vkLogger->debug("Device does not have necessary queue families");
        return false;
    }
    if (!checkDeviceExtensionSupport(device)) {
        vkLogger->debug("Device does not support required extentions");
        return false;
    } 

    return true;
}

bool VulkanEngine::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        if (indices.isComplete())
            break;

        i++;
    }

    return indices;
}