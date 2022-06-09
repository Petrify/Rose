/**
 * @file model_manager.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-01-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once
#include <string>
#include <set>
#include <fstream>
#include <memory>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <assimp/Importer.hpp>
#include <assimp/mesh.h>

#include "vk_mesh.hpp"

struct Model {
    public:
    Mesh mesh;
    // Texture
    // Material
};

class ModelManager {
    private:
    VmaAllocator allocator;    
    public:
    ModelManager(VmaAllocator alloc, VkDevice device);

    void upload(Model& model);
    std::shared_ptr<Model> load(std::string file);
    std::set<std::shared_ptr<Model>> models;
};

