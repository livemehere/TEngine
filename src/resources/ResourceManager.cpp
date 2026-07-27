#include "ResourceManager.h"

#include "../rendering/Renderer.h"
#include "../rendering/mesh/primitives/PrimitiveMeshes.h"

std::filesystem::path ResourceManager::resolvePath(std::filesystem::path filepath) const {
    if (filepath.is_absolute()) {
        return filepath;
    }
    return assetRoot / filepath;
}

/* mesh */
const Mesh & ResourceManager::getPlaneMesh() {
    if (!planeMesh) {
        auto [vertices, indices] = PrimitiveMeshes::createPlane();
        planeMesh = std::make_unique<Mesh>(vertices, indices);
    }
    return *planeMesh;
}

const Mesh & ResourceManager::getCubeMesh() {
    if (!cubeMesh) {
        auto [vertices, indices] = PrimitiveMeshes::createCube();
        cubeMesh = std::make_unique<Mesh>(vertices, indices);
    }
    return *cubeMesh;
}

/* shaders */
const Shader & ResourceManager::getLitShader() {
    if (!litShader) {
        litShader = std::make_unique<Shader>(resolvePath("shaders/basic.vert"), resolvePath("shaders/lit.frag"));
        const auto shader = litShader.get();
        shader->bindUniformBlock("CameraData", UniformBinding::Camera);
        shader->bindUniformBlock("LightsData", UniformBinding::Lights);
    }
    return *litShader;
}

const Shader & ResourceManager::getUnlitShader() {
    if (!unlitShader) {
        unlitShader = std::make_unique<Shader>(resolvePath("shaders/basic.vert"),resolvePath("shaders/unlit.frag"));
        const auto shader = unlitShader.get();
        shader->bindUniformBlock("CameraData", UniformBinding::Camera);
    }
    return *unlitShader;
}


LitMaterial & ResourceManager::loadLitMaterial(const std::string& key, const Shader& shader, const Texture2D& texture) {
    if (const auto it = litMaterials.find(key); it != litMaterials.end()) {
        return *it->second;
    }

    auto material = std::make_unique<LitMaterial>(shader, texture);

    auto [it, inserted] = litMaterials.emplace(key,std::move(material));

    return *it->second;
}

UnlitMaterial & ResourceManager::loadUnlitMaterial(const std::string &key, const Shader &shader,
    const Texture2D &texture) {
    if (const auto it = unlitMaterials.find(key); it != unlitMaterials.end()) {
        return *it->second;
    }

    auto material = std::make_unique<UnlitMaterial>(shader, texture);

    auto [it, inserted] = unlitMaterials.emplace(key,std::move(material));

    return *it->second;
}

const Texture2D & ResourceManager::loadTexture(const std::string &path) {
    if (const auto it = textures.find(path); it != textures.end()) {
        return *it->second;
    }

    std::unique_ptr<Texture2D> texture;
    if (path == "builtin:white") {
        constexpr std::array<uint8_t, 4> pixels{255,255,255,255};
        texture = std::make_unique<Texture2D>(1, 1, pixels);
    } else {
        texture = std::make_unique<Texture2D>(resolvePath(path));
    }

    auto [it, inserted] = textures.emplace(path,std::move(texture));

    return *it->second;
}

const Model & ResourceManager::loadModel(const std::string &path) {
    if (const auto it = models.find(path); it != models.end()) {
        return *it->second;
    }

    auto model = std::make_unique<Model>(resolvePath(path));

    auto [it, inserted] = models.emplace(path,std::move(model));

    return *it->second;
}