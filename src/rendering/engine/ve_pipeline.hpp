#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

struct PipelineBuilder
{
    public: 
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	VkViewport viewport;
	VkRect2D scissor;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkPipelineViewportStateCreateInfo viewportState;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo colorBlending;
	VkPipelineMultisampleStateCreateInfo multisampling;
	VkPipelineDepthStencilStateCreateInfo depthStencil;

	PipelineBuilder() {}
	~PipelineBuilder() {}
    
    void initBasic();

    VkPipeline buildPipeline(VkDevice device, VkRenderPass renderPass, VkPipelineLayout layout);
};

VkShaderModule loadCompiledShader(const std::string &filename, VkDevice device);
VkPipelineShaderStageCreateInfo shaderStageInfo(VkShaderModule module, VkShaderStageFlagBits typeBit, const char* entrypoint = "main");
