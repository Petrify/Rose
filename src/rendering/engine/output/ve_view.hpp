#pragma once
#include <vector>
#include <vulkan/vulkan.h>

#include "ve_pipeline.hpp"

struct View {
public:
    VkRenderPass renderPass;
    PipelineBuilder graphicsPipelineBuilder;
    VkPipeline graphicsPipeline;
    VkPipelineLayout graphicsPipelineLayout;
    std::vector<VkCommandBuffer> commandBuffers;
};