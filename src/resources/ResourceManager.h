#pragma once
#include <memory>
#include <__filesystem/filesystem_error.h>

#include "../graphics/Shader.h"
#include "../graphics/Texture2D.h"
#include "../rendering/mesh/Mesh.h"
#include "../rendering/mesh/materials/LitMaterial.h"
#include "../rendering/mesh/materials/UnlitMaterial.h"
#include "../rendering/model/Model.h"

class UnlitMaterial;

class ResourceManager {
    std::filesystem::path assetRoot;

    /* built in meshes */
    std::unique_ptr<Mesh> planeMesh;
    std::unique_ptr<Mesh> cubeMesh;

    /* resources */
    std::unique_ptr<Shader> litShader;
    std::unique_ptr<Shader> unlitShader;
    std::unordered_map<std::string, std::unique_ptr<Texture2D>> textures;
    std::unordered_map<std::string, std::unique_ptr<Model>> models;

    std::unordered_map<std::string, std::unique_ptr<LitMaterial>> litMaterials;
    std::unordered_map<std::string, std::unique_ptr<UnlitMaterial>> unlitMaterials;


    std::filesystem::path resolvePath(std::filesystem::path filepath) const;


public:
    ResourceManager(std::filesystem::path rootPath) : assetRoot(rootPath) {};
    ~ResourceManager() = default;
    const Mesh& getPlaneMesh();
    const Mesh& getCubeMesh();
    const Shader& getLitShader();
    const Shader& getUnlitShader();
    LitMaterial& loadLitMaterial(const std::string& key, const Shader& shader, const Texture2D& texture);
    UnlitMaterial& loadUnlitMaterial(const std::string& key, const Shader& shader, const Texture2D& texture);

    const Texture2D& loadTexture(const std::string& path);
    const Model& loadModel(const std::string& path);
};
