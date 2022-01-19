/**
 * @file model_manager.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-01-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <RoseLogging.hpp>
#include "models.hpp"
#include <assimp/scene.h>
#include <assimp/postprocess.h>


ModelManager::ModelManager(VmaAllocator alloc, VkDevice device) : allocator(alloc) {

}

std::shared_ptr<Model> ModelManager::load(std::string file) {
    auto model = std::make_shared<Model>();
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(file, 
    aiProcess_CalcTangentSpace       |
    aiProcess_Triangulate            |
    aiProcess_JoinIdenticalVertices  |
    aiProcess_SortByPType            |
    aiProcess_GenNormals             
    );

    // If the import failed, report it
    if (nullptr == scene || !scene->HasMeshes()) {
        getLogger("VulkanEngine/ModelManager")->warn(importer.GetErrorString());
        return nullptr;
    }

    aiMesh* inMesh = scene->mMeshes[0];
    
    for (size_t f = 0; f < inMesh->mNumFaces; f++)
    {
        for (size_t v = 0; v < 3; v++)
        {
            aiVector3D& inVtx = inMesh->mVertices[inMesh->mFaces[f].mIndices[v]];
            aiVector3D& inNorm = inMesh->mNormals[inMesh->mFaces[f].mIndices[v]];
            Vertex vtx;
            vtx.pos.x = inVtx.x;
            vtx.pos.y = inVtx.y;
            vtx.pos.z = inVtx.z;

            vtx.normal.x = inNorm.x;
            vtx.normal.y = inNorm.y;
            vtx.normal.z = inNorm.z;

            vtx.color = vtx.normal;
            model->mesh.vertices.push_back(vtx);
        }
    }
    
    models.insert(model);
    return model;
}

void ModelManager::upload(Model& model) {

}