#include "vk_mesh.hpp"
#include <vector>
#include "ve_loader.hpp"
#include <assimp/mesh.h>

VertexInputDescription Vertex::getVertexDescription() {
    VertexInputDescription description;

	//we will have just 1 vertex buffer binding, with a per-vertex rate
	VkVertexInputBindingDescription mainBinding = {};
	mainBinding.binding = 0;
	mainBinding.stride = sizeof(Vertex);
	mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	description.bindings.push_back(mainBinding);

	//Position will be stored at Location 0
	VkVertexInputAttributeDescription posAttribute = {};
	posAttribute.binding = 0;
	posAttribute.location = 0;
	posAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	posAttribute.offset = offsetof(Vertex, pos);

	 //Normal will be stored at Location 1
	VkVertexInputAttributeDescription normalAttribute = {};
	normalAttribute.binding = 0;
	normalAttribute.location = 1;
	normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	normalAttribute.offset = offsetof(Vertex, normal);

	//Color will be stored at Location 2
	VkVertexInputAttributeDescription colorAttribute = {};
	colorAttribute.binding = 0;
	colorAttribute.location = 2;
	colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	colorAttribute.offset = offsetof(Vertex, color);

	description.attributes.push_back(posAttribute);
	description.attributes.push_back(normalAttribute);
	description.attributes.push_back(colorAttribute);
	return description;
}

Mesh::Mesh(std::string path) {
    const aiScene* mesh = importer.ReadFile(path, 0);
}

Mesh::Mesh() {
    vertices.resize(6);

	//vertex positions
	vertices[0].pos = { 0.5f, 0.5f, 0.0f };
	vertices[1].pos = {-0.5f, 0.5f, 0.0f };
	vertices[2].pos = {0.f, -1.0f, 0.0f };
    vertices[3].pos = {-0.5f, 0.5f, 0.0f };
	vertices[4].pos = {0.f, -1.0f, 0.0f };
	vertices[5].pos = {1.0f, -1.0f, 0.0f };

	vertices[0].color = { 1.f, 0.f, 0.0f }; 
	vertices[1].color = { 0.f, 1.f, 0.0f }; 
	vertices[2].color = { 0.f, 0.f, 1.0f }; 
    vertices[3].color = { 1.f, 0.f, 0.0f }; 
	vertices[4].color = { 0.f, 1.f, 0.0f }; 
	vertices[5].color = { 0.f, 0.f, 1.0f }; 
}
