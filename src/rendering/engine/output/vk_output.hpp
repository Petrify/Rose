#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <set>
#include <memory>
#include <string>

#include "ve_view.hpp"
#include "vk_swapchain.hpp"
#include "vk_engine.hpp"

class VkOutput : public Initializable {
protected:
    VulkanEngine& engine;
    VkExtent2D extent;
    uint32_t frameTimeCap;
public:

    VkOutput(VulkanEngine& engine) : engine(engine) {}; 

    virtual bool checkDeviceRequirements(VkPhysicalDevice physicalDevice) const = 0;
    virtual const std::set<std::string> getRequiredDeviceExtensions() const = 0;

    virtual void aquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) = 0;
    virtual void draw() = 0;
};

class VkGlfwOutput : public virtual VkOutput {
private:

    bool isInit = false;

    const std::set<std::string> requiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    GLFWwindow* window;

    VkSurfaceKHR surface;
    SwapChainSupportDetails swapChainSupport;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> framebuffers;
    VkFormat imageFormat;

    View view;

    std::vector<uint32_t> queueFamilyIndicies;
    std::vector<VkQueue> queues;

    VkQueue presentQueue;
    VkQueue graphicsQueue;
    uint32_t presentQueueFamily;
    uint32_t graphicsQueueFamily;

    std::vector<VkCommandBuffer> commandBuffers;

    // Sync
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    std::vector<std::optional<uint32_t>> _getRequiredQueueFamilies(VkPhysicalDevice physicalDevice) const;
    
    void registerRequirements();
    void createSwapChain();
    void createImageViews();
    void createRenderPass(View& view);
    void createPipelineBuilder(View& view);
    void createGraphicsPipeline(View& view);
    void createFramebuffers();
    void createCommandBuffers();
    void createSyncObjects();

    void updateExtent();

public:
    VkGlfwOutput(GLFWwindow* window, VulkanEngine& engine);
    ~VkGlfwOutput();

    void init();
    void destroy();

    static const std::set<std::string> getRequiredInstanceExtensions();

    bool checkDeviceRequirements(VkPhysicalDevice physicalDevice) const;
    const std::set<std::string> getRequiredDeviceExtensions() const;

    std::set<uint32_t> getRequiredQueueFamilies(VkPhysicalDevice physicalDevice) const;

    void aquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex);
    void draw();
    bool isPresentFamily(const VkQueueFamilyProperties& prop, uint32_t idx, VkPhysicalDevice device); 

    View* newView();
    void destroyView(View& v);
};

