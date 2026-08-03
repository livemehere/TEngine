#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "../graphics/Shader.h"
#include "../graphics/CubeMap.h"
#include "../graphics/Texture2D.h"
#include "../rendering/mesh/Mesh.h"
#include "../rendering/mesh/materials/PhongMaterial.h"
#include "../rendering/mesh/materials/UnlitMaterial.h"
#include "../rendering/model/Model.h"

template<typename T>
struct ResourceEntry {
    std::string name;
    const T *resource = nullptr;
};

class ResourceManager {
    std::filesystem::path assetRoot;

    /* built-in resources */
    std::unique_ptr<Mesh> planeMesh;
    std::unique_ptr<Mesh> cubeMesh;
    std::unique_ptr<Texture2D> whiteTexture;
    std::unique_ptr<Shader> phongShader;
    std::unique_ptr<Shader> unlitShader;
    std::unique_ptr<Shader> outlineShader;
    std::unique_ptr<Shader> postProcessShader;
    std::unique_ptr<Shader> skyboxShader;

    /* externally loaded resources */
    std::unordered_map<std::string, std::unique_ptr<Texture2D> > textures;
    std::unordered_map<std::string, std::unique_ptr<CubeMap> > cubeMaps;
    std::unordered_map<std::string, std::unique_ptr<Model> > models;

    std::unordered_map<std::string, std::unique_ptr<PhongMaterial> > phongMaterials;
    std::unordered_map<std::string, std::unique_ptr<UnlitMaterial> > unlitMaterials;

    std::vector<ResourceEntry<Mesh>> meshCatalog;
    std::vector<ResourceEntry<Material>> materialCatalog;

    std::filesystem::path resolvePath(std::filesystem::path filepath) const;

public:
    ResourceManager(std::filesystem::path rootPath) : assetRoot(rootPath) {}
    ~ResourceManager() = default;

    const Mesh &getPlaneMesh();

    const Mesh &getCubeMesh();

    std::span<const ResourceEntry<Mesh>> getMeshResources();

    std::span<const ResourceEntry<Material>> getMaterialResources() const;

    Material *findMutableMaterial(const Material *material);

    const Texture2D &getWhiteTexture();

    const Shader &getPhongShader();

    const Shader &getUnlitShader();

    const Shader &getOutlineShader();

    const Shader &getPostProcessShader();

    const Shader &getSkyboxShader();

    PhongMaterial &loadPhongMaterial(const std::string &key, const Shader &shader, const Texture2D &texture);

    UnlitMaterial &loadUnlitMaterial(const std::string &key, const Shader &shader, const Texture2D &texture);

    const Texture2D &loadTexture(const std::string &path);

    const Texture2D &loadEncodedTexture(
        const std::string &key,
        std::span<const std::uint8_t> encodedData
    );

    const Texture2D &loadRawTexture(
        const std::string &key,
        int width,
        int height,
        std::span<const std::uint8_t> rgbaPixels
    );

    const CubeMap &loadCubeMap(
        const std::string& key,
        std::span<const std::string> faces
    );

    const Model &loadModel(const std::string &path, bool flipUVs);
};
