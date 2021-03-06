#pragma once

// std
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <array>
#include <string>
#include <map>
#include <functional> 
#include <stack>

// External Dependencies
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vk_mem_alloc.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h> 

// Local Dependencies
#include <RoseLogging.hpp>

#include "vk_mesh.hpp"
#include "models.hpp"
#include "ve_types.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

const std::vector<const char*> engineInstanceExtensions = {}; // Instance extensions required by the Engine
const std::vector<const char*> engineDeviceExtensions = {}; // Device extensions required by the Engine

typedef std::function<bool(const VkQueueFamilyProperties&, uint32_t, VkPhysicalDevice)> queueCriteriaFunc;

struct AllocatedImage {
    VkImage _image;
    VmaAllocation _allocation;
};

class VulkanEngine
{
public:

    // Constants
    // Vulkan validation layers to enable
    const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    bool initialized = false;

    // Logging
    Logger vkLogger;

    // Vulkan instance
    VkInstance vkInstance;

    // Messaging for Validation Layers  
    VkDebugUtilsMessengerEXT debugMessenger;  

    std::vector<std::string> instanceExtensions; 
    
    std::set<std::string> deviceExtensions; // Vulkan device extentions to enable
    
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkPipelineCache pipelineCache;
    VkDescriptorPool descriptorPool;

    VmaAllocator allocator;

    std::vector<Mesh> meshes; 
    std::unique_ptr<ModelManager> modelMan;

    std::vector<std::function<bool(VkPhysicalDevice)>> deviceReqCallbacks;

    std::vector<queueCriteriaFunc> queueRequirements;
    std::vector<VkQueue*> queues;
    std::vector<uint32_t*> queueFamilies;

    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamily;

    VkCommandPool commandPool;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    std::vector<Initializable*> postInitObjects;
    std::stack<std::function<void()>> destroyStack;

    ImGuiIO* imguiIO;

    std::vector<const char *> getInstanceExtensions();

    void createInstance();
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createCommandPool();
    void createVertexBuffer();
    void createMemoryAllocator();
    void createDescriptorPool();
    void createPipelineCache();
    void initImgui();
    
    void loadMeshes();
    void uploadMesh(Mesh& mesh);
    
    void compileInstanceExtensions(std::set<std::string> external);
    void compileDeviceExtensions();
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    bool isDeviceSuitable(VkPhysicalDevice device);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    VulkanEngine(std::set<std::string> instanceExtensions);
    ~VulkanEngine();

    void init();
    void destroy();

    void start();
    void stop();

    bool isInit();

    void deferDestroy(std::function<void()> foo);

    void requestDeviceRequirement(std::function<bool(VkPhysicalDevice)> condition);
    void requestDeviceExtensions(std::set<std::string> extensions);

    void requestQueue(queueCriteriaFunc criteria, uint32_t* familyIdx, VkQueue* queue);

    void requestPostInitialization(Initializable* obj);

    bool checkQueueCompatibility(VkPhysicalDevice device);
    void compileQueueFamilyIndicies();
    void fulfillQueueRequests();
};

bool isGraphicsFamily(const VkQueueFamilyProperties& prop); 

void check_vk_result(VkResult err);





