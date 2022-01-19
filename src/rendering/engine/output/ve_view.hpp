#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "ve_pipeline.hpp"
struct View {
public:
    glm::vec3 cameraPos;
    glm::vec3 cameraLook;
    glm::vec2 fov;
    VkRenderPass renderPass;
    PipelineBuilder graphicsPipelineBuilder;
    VkPipeline graphicsPipeline;
    VkPipelineLayout graphicsPipelineLayout;
    std::vector<VkCommandBuffer> commandBuffers;
    VkClearValue clearColor = {{{0.2f, 0.2f, 0.2f, 1.0f}}};
};