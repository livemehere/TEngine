#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>

#include "../graphics/Shader.h"
#include "../graphics/Texture2D.h"
#include "../rendering/mesh/Mesh.h"
#include "../rendering/mesh/materials/LitMaterial.h"
#include "../rendering/mesh/materials/UnlitMaterial.h"
#include "../rendering/model/Model.h"

class ResourceManager {
    std::filesystem::path assetRoot;

    /* built-in resources */
    std::unique_ptr<Mesh> planeMesh;
    std::unique_ptr<Mesh> cubeMesh;
    std::unique_ptr<Texture2D> whiteTexture;
    std::unique_ptr<Shader> litShader;
    std::unique_ptr<Shader> unlitShader;
    std::unique_ptr<Shader> outlineShader;
    std::unique_ptr<Shader> postProcessShader;

    /* externally loaded resources */
    std::unordered_map<std::string, std::unique_ptr<Texture2D> > textures;
    std::unordered_map<std::string, std::unique_ptr<Model> > models;

    std::unordered_map<std::string, std::unique_ptr<LitMaterial> > litMaterials;
    std::unordered_map<std::string, std::unique_ptr<UnlitMaterial> > unlitMaterials;


    std::filesystem::path resolvePath(std::filesystem::path filepath) const;

public:
    ResourceManager(std::filesystem::path rootPath) : assetRoot(rootPath) {}
    ~ResourceManager() = default;

    const Mesh &getPlaneMesh();

    const Mesh &getCubeMesh();

    const Texture2D &getWhiteTexture();

    const Shader &getLitShader();

    const Shader &getUnlitShader();

    const Shader &getOutlineShader();

    const Shader &getPostProcessShader();

    LitMaterial &loadLitMaterial(const std::string &key, const Shader &shader, const Texture2D &texture);

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

    const Model &loadModel(const std::string &path, bool flipUVs);
};
