#include "ResourceManager.h"

#include <algorithm>
#include <array>
#include <format>
#include <vector>

#include "../rendering/Renderer.h"
#include "../rendering/mesh/primitives/PrimitiveMeshes.h"
#include "../rendering/model/ModelImporter.h"

namespace {
    std::string textureCacheKey(
        const std::string &sourceKey,
        const TextureColorSpace colorSpace
    ) {
        return sourceKey + (
            colorSpace == TextureColorSpace::SRGB
                ? "#srgb"
                : "#linear"
        );
    }

    template<typename T>
    void registerResource(
        std::vector<ResourceEntry<T>> &catalog,
        std::string name,
        const T &resource
    ) {
        const auto existing = std::ranges::find(
            catalog,
            name,
            &ResourceEntry<T>::name
        );
        if (existing != catalog.end()) {
            existing->resource = &resource;
            return;
        }

        catalog.push_back({
            .name = std::move(name),
            .resource = &resource
        });
    }
}

std::filesystem::path ResourceManager::resolvePath(std::filesystem::path filepath) const {
    if (filepath.is_absolute()) {
        return filepath;
    }
    return assetRoot / filepath;
}

/* mesh */
const Mesh &ResourceManager::getPlaneMesh() {
    if (!planeMesh) {
        auto [vertices, indices] = PrimitiveMeshes::createPlane();
        planeMesh = std::make_unique<Mesh>(vertices, indices);
        registerResource(meshCatalog, "Built-in/Plane", *planeMesh);
    }
    return *planeMesh;
}

const Mesh &ResourceManager::getCubeMesh() {
    if (!cubeMesh) {
        auto [vertices, indices] = PrimitiveMeshes::createCube();
        cubeMesh = std::make_unique<Mesh>(vertices, indices);
        registerResource(meshCatalog, "Built-in/Cube", *cubeMesh);
    }
    return *cubeMesh;
}

std::span<const ResourceEntry<Mesh>> ResourceManager::getMeshResources() {
    getPlaneMesh();
    getCubeMesh();
    return meshCatalog;
}

std::span<const ResourceEntry<Material>>
ResourceManager::getMaterialResources() const {
    return materialCatalog;
}

std::span<const ResourceEntry<Model>>
ResourceManager::getModelResources() const {
    return modelCatalog;
}

Material *ResourceManager::findMutableMaterial(const Material *material) {
    if (!material) {
        return nullptr;
    }

    for (auto &[key, candidate] : phongMaterials) {
        if (candidate.get() == material) {
            return candidate.get();
        }
    }

    for (auto &[key, candidate] : unlitMaterials) {
        if (candidate.get() == material) {
            return candidate.get();
        }
    }

    return nullptr;
}

const Texture2D &ResourceManager::getWhiteTexture() {
    if (!whiteTexture) {
        constexpr std::array<std::uint8_t, 4> pixels{255, 255, 255, 255};
        whiteTexture = std::make_unique<Texture2D>(
            1,
            1,
            pixels,
            TextureColorSpace::SRGB
        );
    }
    return *whiteTexture;
}

/* shaders */
const Shader &ResourceManager::getPhongShader() {
    if (!phongShader) {
        phongShader = std::make_unique<Shader>(resolvePath("shaders/basic.vert"), resolvePath("shaders/phong.frag"));
        const auto shader = phongShader.get();
        shader->bindUniformBlock("CameraData", UniformBinding::Camera);
        shader->bindUniformBlock("LightsData", UniformBinding::Lights);
        shader->bindUniformBlock("DebugData", UniformBinding::Debug);
    }
    return *phongShader;
}

const Shader &ResourceManager::getUnlitShader() {
    if (!unlitShader) {
        unlitShader = std::make_unique<Shader>(resolvePath("shaders/basic.vert"), resolvePath("shaders/unlit.frag"));
        const auto shader = unlitShader.get();
        shader->bindUniformBlock("CameraData", UniformBinding::Camera);
        shader->bindUniformBlock("DebugData", UniformBinding::Debug);
    }
    return *unlitShader;
}

const Shader &ResourceManager::getOutlineShader() {
    if (!outlineShader) {
        outlineShader = std::make_unique<Shader>(
            resolvePath("shaders/outline.vert"),
            resolvePath("shaders/outline.frag")
        );
        const auto shader = outlineShader.get();
        shader->bindUniformBlock("CameraData", UniformBinding::Camera);
    }
    return *outlineShader;
}

const Shader & ResourceManager::getPostProcessShader() {
    if (!postProcessShader) {
        postProcessShader = std::make_unique<Shader>(
            resolvePath("shaders/postProcess.vert"),
            resolvePath("shaders/postProcess.frag")
        );
    }
    return *postProcessShader;

}

const Shader &ResourceManager::getBloomExtractShader() {
    if (!bloomExtractShader) {
        bloomExtractShader = std::make_unique<Shader>(
            resolvePath("shaders/postProcess.vert"),
            resolvePath("shaders/bloomExtract.frag")
        );
    }
    return *bloomExtractShader;
}

const Shader &ResourceManager::getGaussianBlurShader() {
    if (!gaussianBlurShader) {
        gaussianBlurShader = std::make_unique<Shader>(
            resolvePath("shaders/postProcess.vert"),
            resolvePath("shaders/gaussianBlur.frag")
        );
    }
    return *gaussianBlurShader;
}

const Shader &ResourceManager::getDeferredGeometryShader() {
    if (!deferredGeometryShader) {
        deferredGeometryShader = std::make_unique<Shader>(
            resolvePath("shaders/basic.vert"),
            resolvePath("shaders/deferredGeometry.frag")
        );
        deferredGeometryShader->bindUniformBlock(
            "CameraData",
            UniformBinding::Camera
        );
    }
    return *deferredGeometryShader;
}

const Shader &ResourceManager::getDeferredLightingShader() {
    if (!deferredLightingShader) {
        deferredLightingShader = std::make_unique<Shader>(
            resolvePath("shaders/postProcess.vert"),
            resolvePath("shaders/deferredLighting.frag")
        );
        deferredLightingShader->bindUniformBlock(
            "CameraData",
            UniformBinding::Camera
        );
        deferredLightingShader->bindUniformBlock(
            "LightsData",
            UniformBinding::Lights
        );
        deferredLightingShader->bindUniformBlock(
            "DebugData",
            UniformBinding::Debug
        );
    }
    return *deferredLightingShader;
}

const Shader &ResourceManager::getSSAOShader() {
    if (!ssaoShader) {
        ssaoShader = std::make_unique<Shader>(
            resolvePath("shaders/postProcess.vert"),
            resolvePath("shaders/ssao.frag")
        );
        ssaoShader->bindUniformBlock(
            "CameraData",
            UniformBinding::Camera
        );
    }
    return *ssaoShader;
}

const Shader &ResourceManager::getSSAOBlurShader() {
    if (!ssaoBlurShader) {
        ssaoBlurShader = std::make_unique<Shader>(
            resolvePath("shaders/postProcess.vert"),
            resolvePath("shaders/ssaoBlur.frag")
        );
    }
    return *ssaoBlurShader;
}

const Shader &ResourceManager::getFrameBufferDebugShader() {
    if (!frameBufferDebugShader) {
        frameBufferDebugShader = std::make_unique<Shader>(
            resolvePath("shaders/postProcess.vert"),
            resolvePath("shaders/framebufferDebug.frag")
        );
    }
    return *frameBufferDebugShader;
}

const Shader &ResourceManager::getSkyboxShader() {
    if (!skyboxShader) {
        skyboxShader = std::make_unique<Shader>(
            resolvePath("shaders/skybox.vert"),
            resolvePath("shaders/skybox.frag")
        );
        skyboxShader->bindUniformBlock("CameraData", UniformBinding::Camera);

        GLint previousProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        skyboxShader->use();
        skyboxShader->setInt("uSkybox", 0);
        glUseProgram(previousProgram);
    }
    return *skyboxShader;
}

const Shader &ResourceManager::getNormalDebugShader() {
    if (!normalDebugShader) {
        normalDebugShader = std::make_unique<Shader>(
            resolvePath("shaders/normalDebug.vert"),
            resolvePath("shaders/normalDebug.geom"),
            resolvePath("shaders/normalDebug.frag")
        );
        normalDebugShader->bindUniformBlock(
            "CameraData",
            UniformBinding::Camera
        );
    }
    return *normalDebugShader;
}

const Shader &ResourceManager::getShadowDepthShader() {
    if (!shadowDepthShader) {
        shadowDepthShader = std::make_unique<Shader>(
            resolvePath("shaders/shadowDepth.vert"),
            resolvePath("shaders/shadowDepth.frag")
        );
    }
    return *shadowDepthShader;
}

const Shader &ResourceManager::getPointShadowDepthShader() {
    if (!pointShadowDepthShader) {
        pointShadowDepthShader = std::make_unique<Shader>(
            resolvePath("shaders/pointShadowDepth.vert"),
            resolvePath("shaders/pointShadowDepth.geom"),
            resolvePath("shaders/pointShadowDepth.frag")
        );
    }
    return *pointShadowDepthShader;
}

PhongMaterial &ResourceManager::loadPhongMaterial(const std::string &key, const Shader &shader, const Texture2D &texture) {
    if (const auto it = phongMaterials.find(key); it != phongMaterials.end()) {
        return *it->second;
    }

    auto material = std::make_unique<PhongMaterial>(shader, texture);

    auto [it, inserted] = phongMaterials.emplace(key, std::move(material));

    registerResource(
        materialCatalog,
        "Phong/" + key,
        static_cast<const Material &>(*it->second)
    );

    return *it->second;
}

UnlitMaterial &ResourceManager::loadUnlitMaterial(const std::string &key, const Shader &shader,
                                                  const Texture2D &texture) {
    if (const auto it = unlitMaterials.find(key); it != unlitMaterials.end()) {
        return *it->second;
    }

    auto material = std::make_unique<UnlitMaterial>(shader, texture);

    auto [it, inserted] = unlitMaterials.emplace(key, std::move(material));

    registerResource(
        materialCatalog,
        "Unlit/" + key,
        static_cast<const Material &>(*it->second)
    );

    return *it->second;
}

const Texture2D &ResourceManager::loadTexture(
    const std::string &path,
    const TextureColorSpace colorSpace
) {
    const std::string resolvedPath =
            resolvePath(path).lexically_normal().string();
    const std::string key = textureCacheKey(resolvedPath, colorSpace);

    if (const auto it = textures.find(key); it != textures.end()) {
        return *it->second;
    }

    auto texture = std::make_unique<Texture2D>(resolvedPath, colorSpace);

    auto [it, inserted] = textures.emplace(key, std::move(texture));

    return *it->second;
}

const Texture2D &ResourceManager::loadEncodedTexture(
    const std::string &key,
    std::span<const std::uint8_t> encodedData,
    const TextureColorSpace colorSpace
) {
    const std::string cacheKey = textureCacheKey(key, colorSpace);
    if (const auto it = textures.find(cacheKey);
        it != textures.end()) {
        return *it->second;
    }

    auto texture =
            std::make_unique<Texture2D>(encodedData, colorSpace);

    auto [it, inserted] = textures.emplace(
        cacheKey,
        std::move(texture)
    );

    return *it->second;
}

const Texture2D &ResourceManager::loadRawTexture(
    const std::string &key,
    int width,
    int height,
    std::span<const std::uint8_t> rgbaPixels,
    const TextureColorSpace colorSpace
) {
    const std::string cacheKey = textureCacheKey(key, colorSpace);
    if (const auto it = textures.find(cacheKey);
        it != textures.end()) {
        return *it->second;
    }

    auto texture = std::make_unique<Texture2D>(
        width,
        height,
        rgbaPixels,
        colorSpace
    );

    auto [it, inserted] = textures.emplace(
        cacheKey,
        std::move(texture)
    );

    return *it->second;
}

const CubeMap &ResourceManager::loadCubeMap(
    const std::string& key,
    std::span<const std::string> faces,
    const TextureColorSpace colorSpace
) {
    const std::string cacheKey = textureCacheKey(key, colorSpace);
    if (const auto it = cubeMaps.find(cacheKey); it != cubeMaps.end()) {
        return *it->second;
    }

    std::vector<std::string> resolvedFaces;
    resolvedFaces.reserve(faces.size());
    for (const std::string& face : faces) {
        resolvedFaces.push_back(resolvePath(face).lexically_normal().string());
    }

    auto cubeMap = std::make_unique<CubeMap>(resolvedFaces, colorSpace);
    auto [it, inserted] = cubeMaps.emplace(cacheKey, std::move(cubeMap));
    return *it->second;
}

const Model &ResourceManager::loadModel(
    const std::string &path,
    bool flipUVs
) {
    const std::filesystem::path resolvedPath =
            resolvePath(path).lexically_normal();

    const std::string key =
            resolvedPath.string() +
            (flipUVs ? "#flipUV" : "#keepUV");

    if (const auto it = models.find(key);
        it != models.end()) {
        return *it->second;
    }

    ModelImporter importer{*this};

    std::unique_ptr<Model> model =
            importer.import(
                resolvedPath,
                flipUVs
            );

    auto [it, inserted] = models.emplace(
        key,
        std::move(model)
    );

    registerResource(
        modelCatalog,
        path + (flipUVs ? " [Flipped UV]" : ""),
        *it->second
    );

    for (size_t partIndex = 0;
         partIndex < it->second->parts.size();
         ++partIndex) {
        const ModelPart &part = it->second->parts[partIndex];
        if (!part.mesh) {
            continue;
        }

        registerResource(
            meshCatalog,
            std::format(
                "{}/Mesh {}{}",
                path,
                partIndex,
                flipUVs ? " [Flipped UV]" : ""
            ),
            *part.mesh
        );
    }

    return *it->second;
}
