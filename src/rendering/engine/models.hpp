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
#include "vk_mesh.hpp"
#include <string>
#include <set>
#include <vk_mem_alloc.h>

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
    ModelManager(VmaAllocator alloc) : allocator(alloc) {}

    Model& loadModel(char* file);
    std::set<Model> models;
};