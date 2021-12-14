#pragma once

#include <vulkan/vulkan.h>
#include <optional>

struct QueueFamilyIndices
{
    public:
    std::optional<uint32_t> graphicsFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value();
    }
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);