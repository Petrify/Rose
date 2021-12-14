#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <set>
#include <memory>
#include <string>

#include "ve_pipeline.hpp"
#include "vk_queue.hpp"
#include "vk_swapchain.hpp"

class VkOutput{
protected:
    VkExtent2D extent;
    VkInstance instance;
    uint32_t frameTimeCap;
public:
    
    // Pre Init
    virtual bool checkDeviceRequirements(VkPhysicalDevice physicalDevice) const = 0;
    virtual std::vector<uint32_t> getRequiredQueueFamilies(VkPhysicalDevice physicalDevice) const = 0;
    virtual const std::vector<std::string> getRequiredExtensions() const = 0;

    virtual void bind(VkInstance instance) = 0;
    virtual void unbind() = 0;
    virtual void init(VkDevice device, VkPhysicalDevice physicalDevice, std::vector<uint32_t> queueIndicies) = 0;

    // Post Init
    virtual void aquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) = 0;
    virtual void draw() = 0;
};

typedef std::weak_ptr<VkOutput> VkOutputHandle; 

class VkGlfwOutput : public virtual VkOutput {
private:

    bool isInit = false;

    const std::set<std::string> requiredExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    GLFWwindow* window;

    VkSurfaceKHR surface;
    SwapChainSupportDetails swapChainSupport;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> framebuffers;

    VkPhysicalDevice physicalDevice;
    VkFormat imageFormat;
    VkDevice device;
    VkRenderPass renderPass;
    PipelineBuilder graphicsPipelineBuilder;
    VkPipeline graphicsPipeline;
    VkPipelineLayout graphicsPipelineLayout;

    std::vector<uint32_t> queueFamilyIndicies;
    std::vector<VkQueue> queues;

    enum QueueRef : uint32_t {
        presentIdx,
        graphicsIdx,
        _count // number of entries in Enum. must always be last
    };

    std::vector<std::optional<uint32_t>> _getRequiredQueueFamilies(VkPhysicalDevice physicalDevice) const;
    
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createPipelineBuilders();
    void updateExtent();
    void createGraphicsPipeline();
    void createFramebuffers();

public:
    VkGlfwOutput(GLFWwindow* window);
    ~VkGlfwOutput();

    void bind(VkInstance instance);
    void init(VkDevice device, VkPhysicalDevice physicalDevice, std::vector<uint32_t> queueIndicies);
    void unbind();

    bool checkDeviceRequirements(VkPhysicalDevice physicalDevice) const;
    std::vector<uint32_t> getRequiredQueueFamilies(VkPhysicalDevice physicalDevice) const;
    const std::vector<std::string> getRequiredExtensions() const;
    
    void aquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex);
    void draw();
};