#include "vma_inc.hpp"
#include "vk_engine.hpp"
#include "ve_types.hpp"

#include "vk_debug.hpp"
#include "vk_swapchain.hpp"
#include "ve_pipeline.hpp"

VulkanEngine::VulkanEngine(std::set<std::string> instanceExtensions)
{
    vkLogger = getLogger("VulkanEngine");
    compileInstanceExtensions(instanceExtensions);
    createInstance();
    setupDebugMessenger();
    requestQueue([&] (const VkQueueFamilyProperties& prop, uint32_t idx, VkPhysicalDevice device) -> bool {return isGraphicsFamily(prop);}, &graphicsQueueFamily, &graphicsQueue);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    imguiIO = &ImGui::GetIO();
    ImGui::StyleColorsDark();
}

VulkanEngine::~VulkanEngine()
{
    while (!destroyStack.empty()) {
        destroyStack.top()();
        destroyStack.pop();
    }
    
    ImGui::DestroyContext();
    if (enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);
    }

    vkDestroyInstance(vkInstance, nullptr);
    vkLogger->flush();
}

void VulkanEngine::compileInstanceExtensions(std::set<std::string> external) {
    // Populate instance Extensions
    std::set<std::string> extensions = {};
    for(auto ext : external) {
        extensions.insert(ext);
    }

    for(auto ext : engineInstanceExtensions) {
        extensions.insert(ext);
    }

    // Validation Layers
    if (enableValidationLayers)
    {
        extensions.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    for(auto ext : extensions)
    {
        instanceExtensions.push_back(ext); 
    }
}

void VulkanEngine::init() {
    vkLogger->info("Initializing Vulkan Render Engine");

    compileDeviceExtensions();
    pickPhysicalDevice();
    createLogicalDevice();
    createMemoryAllocator();
    createCommandPool();
    createPipelineCache();
    createDescriptorPool();
    initImgui();
    loadMeshes();

    for(auto obj : postInitObjects)
    {
        obj->init();
    }
}

void VulkanEngine::destroy() {

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    for(auto mesh : meshes)
    {
        vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
    }
    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyPipelineCache(device, pipelineCache, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    //for(auto output : outputs) {removeOutput(output);}
    vmaDestroyAllocator(allocator);
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

    auto extensions = getInstanceExtensions();
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

void VulkanEngine::requestDeviceRequirement(std::function<bool(VkPhysicalDevice)> condition) {
    deviceReqCallbacks.push_back(condition);
}
void VulkanEngine::requestDeviceExtensions(std::set<std::string> extensions) {
    deviceExtensions.insert(extensions.begin(), extensions.end());
}

void  VulkanEngine::requestQueue(queueCriteriaFunc criteria, uint32_t* familyIdx, VkQueue* queue) {
    queueRequirements.push_back(criteria);
    queues.push_back(queue);
    queueFamilies.push_back(familyIdx); //just to keep size
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
            vkLogger->info("Selected Device: {}", properties.deviceName);
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
    for(const auto& ext : engineDeviceExtensions)
    {
        deviceExtensions.insert(ext);
    }
}

void VulkanEngine::createLogicalDevice()
{
    compileQueueFamilyIndicies();
    std::set<uint32_t> uniqueQueueFamilies;
    for(auto familyPtr : queueFamilies)
    {
        uniqueQueueFamilies.insert(*familyPtr);
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
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

    std::vector<const char*> extensions;
    extensions.reserve(deviceExtensions.size());
    for(auto& ext : deviceExtensions)    
    {
        vkLogger->debug("Requesting Device Extension: {}", ext);
        extensions.push_back(ext.data());
    }

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

    fulfillQueueRequests();
}

void VulkanEngine::createCommandPool() 
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsQueueFamily;
    poolInfo.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void VulkanEngine::createVertexBuffer() 
{

}

std::vector<const char *> VulkanEngine::getInstanceExtensions()
{
    std::vector<const char *> extensions;

    for (auto& ext : instanceExtensions) {
        extensions.push_back(ext.data());
        vkLogger->debug("Requesting Instance Extension: {}", ext);
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

    checkQueueCompatibility(device);
    
    for(auto func : deviceReqCallbacks)
    {
        if (!func(device)) {
            vkLogger->debug("Device does not fulfill external Requirements");
            return false;
        }   
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

bool VulkanEngine::checkQueueCompatibility(VkPhysicalDevice device) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    std::vector<bool> familyFound(queues.size(), false);
    uint32_t familyIdx = 0;
    for (const auto &queueFamily : queueFamilies)
    {
        for (size_t reqIdx = 0; reqIdx < queues.size(); reqIdx++)
        {
            if (!familyFound[reqIdx] && queueRequirements[reqIdx](queueFamily, familyIdx, device)) {
                familyFound[reqIdx] = true;
            }
        }
        familyIdx++;
    }

    for(auto& b : familyFound)
    {
        if(!b) {return false;}
    }

    return true;
}

void VulkanEngine::compileQueueFamilyIndicies() {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> deviceQueueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, deviceQueueFamilies.data());

    std::vector<bool> familyFound(queues.size(), false);
    uint32_t familyIdx = 0;
    for (const auto &queueFamily : deviceQueueFamilies)
    {
        for (size_t reqIdx = 0; reqIdx < queues.size(); reqIdx++)
        {
            if (!familyFound[reqIdx] && queueRequirements[reqIdx](queueFamily, familyIdx, physicalDevice)) {
                familyFound[reqIdx] = true;
                *queueFamilies[reqIdx] = familyIdx;
            }
        }
        familyIdx++;
    }
}

void VulkanEngine::fulfillQueueRequests() {
    for (size_t queueIdx = 0; queueIdx < queues.size(); queueIdx++)
    {
        vkGetDeviceQueue(device, *queueFamilies[queueIdx], 0, queues[queueIdx]);
    }
}

void VulkanEngine::requestPostInitialization(Initializable* obj) {
    postInitObjects.push_back(obj);
}

bool VulkanEngine::isInit() {
    return initialized;
}

void VulkanEngine::createMemoryAllocator() {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = vkInstance;
    vmaCreateAllocator(&allocatorInfo, &allocator);
}

void VulkanEngine::loadMeshes() {
    modelMan = std::make_unique<ModelManager>(allocator, device);

    auto teapot = modelMan->load("../assets/teapot.obj");

    meshes.push_back(teapot->mesh);
    uploadMesh(meshes.back());
}

void VulkanEngine::uploadMesh(Mesh& mesh)
{
	//allocate vertex buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	//this is the total size, in bytes, of the buffer we are allocating
	bufferInfo.size = mesh.vertices.size() * sizeof(Vertex);
	//this buffer is going to be used as a Vertex Buffer
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    vkLogger->debug("Allocating Vertex buffer: {} Verticies ({} bytes)", mesh.vertices.size(), bufferInfo.size);

	//let the VMA library know that this data should be writeable by CPU, but also readable by GPU
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	//allocate the buffer
	auto result = (vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo,
		&mesh.vertexBuffer.buffer,
		&mesh.vertexBuffer.allocation,
		nullptr));
    
    if(result != VK_SUCCESS) {}

    //copy vertex data
	void* data;
	vmaMapMemory(allocator, mesh.vertexBuffer.allocation, &data);

	memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));

	vmaUnmapMemory(allocator, mesh.vertexBuffer.allocation);
}

void VulkanEngine::deferDestroy(std::function<void()> foo) {
    destroyStack.push(foo);
}

void VulkanEngine::createDescriptorPool() {
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    check_vk_result(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool));
}

void VulkanEngine::createPipelineCache() {
    VkPipelineCacheCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    ci.flags = 0;

    vkCreatePipelineCache(device, &ci, nullptr, &pipelineCache);
}

bool isGraphicsFamily(const VkQueueFamilyProperties& prop) {
    return prop.queueFlags & VK_QUEUE_GRAPHICS_BIT;
}

void check_vk_result(VkResult err)
{
    static auto logger = getLogger("VulkanEngine");
    if (err == 0)
        return;
    logger->critical("Vulkan error: #{}", err);
    if (err < 0)
        abort();
}

void VulkanEngine::initImgui() {
    
}