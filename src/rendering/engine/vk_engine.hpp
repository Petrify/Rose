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

// External Dependencies
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

// Local Dependencies
#include "logging/logging.hpp"

#include "vk_queue.hpp"
#include "vk_mesh.hpp"
//#include "output/vk_output.hpp"

const int MAX_FRAMES_IN_FLIGHT = 2;
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

typedef bool(*queueCondition)();
const std::vector<const char*> engineInstanceExtensions = {}; // Instance extensions required by the Engine

class VulkanEngine
{
public:

    // Constants
    // Vulkan validation layers to enable
    const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    // Verticies
    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 0.2f}}
    };

    bool initialized;

    // Logging
    Logger vkLogger;

    // Vulkan instance
    VkInstance vkInstance;

    // Messaging for Validation Layers  
    VkDebugUtilsMessengerEXT debugMessenger;  

    bool useGlfw;
    
    std::vector<std::string> deviceExtensions; // Vulkan device extentions to enable
    std::vector<const char*> _deviceExtensions; // stores c_str references to strings in deviceExtensions
    
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    std::map<bool (*)(), uint32_t> queueReqs;
    std::map<uint32_t, VkQueue> queues;
    QueueFamilyIndices queueIndicies;
    VkQueue graphicsQueue;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    std::vector<const char *> getInstanceExtensions();

    void createInstance();
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void initOutputs();
    void createCommandPool();
    void createVertexBuffer();
    
    void compileDeviceExtensions();
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    bool isDeviceSuitable(VkPhysicalDevice device);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    VulkanEngine(std::vector<const char*> instanceExtensions);
    ~VulkanEngine();

    void init();
    void shutdown();

    void start();
    void stop();

    void addView();
    void removeView();

    void requestDeviceRequirement();
    void requestDeviceExtension();

    void requestQueue(queueCondition cond);
    VkQueue getQueue(queueCondition cond);
};







