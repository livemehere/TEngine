#include "ResourceManager.h"

#include <array>
#include <vector>

#include "../rendering/Renderer.h"
#include "../rendering/mesh/primitives/PrimitiveMeshes.h"
#include "../rendering/model/ModelImporter.h"

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
    }
    return *planeMesh;
}

const Mesh &ResourceManager::getCubeMesh() {
    if (!cubeMesh) {
        auto [vertices, indices] = PrimitiveMeshes::createCube();
        cubeMesh = std::make_unique<Mesh>(vertices, indices);
    }
    return *cubeMesh;
}

const Texture2D &ResourceManager::getWhiteTexture() {
    if (!whiteTexture) {
        constexpr std::array<std::uint8_t, 4> pixels{255, 255, 255, 255};
        whiteTexture = std::make_unique<Texture2D>(1, 1, pixels);
    }
    return *whiteTexture;
}

/* shaders */
const Shader &ResourceManager::getLitShader() {
    if (!litShader) {
        litShader = std::make_unique<Shader>(resolvePath("shaders/basic.vert"), resolvePath("shaders/lit.frag"));
        const auto shader = litShader.get();
        shader->bindUniformBlock("CameraData", UniformBinding::Camera);
        shader->bindUniformBlock("LightsData", UniformBinding::Lights);
    }
    return *litShader;
}

const Shader &ResourceManager::getUnlitShader() {
    if (!unlitShader) {
        unlitShader = std::make_unique<Shader>(resolvePath("shaders/basic.vert"), resolvePath("shaders/unlit.frag"));
        const auto shader = unlitShader.get();
        shader->bindUniformBlock("CameraData", UniformBinding::Camera);
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

LitMaterial &ResourceManager::loadLitMaterial(const std::string &key, const Shader &shader, const Texture2D &texture) {
    if (const auto it = litMaterials.find(key); it != litMaterials.end()) {
        return *it->second;
    }

    auto material = std::make_unique<LitMaterial>(shader, texture);

    auto [it, inserted] = litMaterials.emplace(key, std::move(material));

    return *it->second;
}

UnlitMaterial &ResourceManager::loadUnlitMaterial(const std::string &key, const Shader &shader,
                                                  const Texture2D &texture) {
    if (const auto it = unlitMaterials.find(key); it != unlitMaterials.end()) {
        return *it->second;
    }

    auto material = std::make_unique<UnlitMaterial>(shader, texture);

    auto [it, inserted] = unlitMaterials.emplace(key, std::move(material));

    return *it->second;
}

const Texture2D &ResourceManager::loadTexture(const std::string &path) {
    const std::string key = resolvePath(path).lexically_normal().string();

    if (const auto it = textures.find(key); it != textures.end()) {
        return *it->second;
    }

    auto texture = std::make_unique<Texture2D>(key);

    auto [it, inserted] = textures.emplace(key, std::move(texture));

    return *it->second;
}

const Texture2D &ResourceManager::loadEncodedTexture(
    const std::string &key,
    std::span<const std::uint8_t> encodedData
) {
    if (const auto it = textures.find(key);
        it != textures.end()) {
        return *it->second;
    }

    auto texture =
            std::make_unique<Texture2D>(encodedData);

    auto [it, inserted] = textures.emplace(
        key,
        std::move(texture)
    );

    return *it->second;
}

const Texture2D &ResourceManager::loadRawTexture(
    const std::string &key,
    int width,
    int height,
    std::span<const std::uint8_t> rgbaPixels
) {
    if (const auto it = textures.find(key);
        it != textures.end()) {
        return *it->second;
    }

    auto texture = std::make_unique<Texture2D>(
        width,
        height,
        rgbaPixels
    );

    auto [it, inserted] = textures.emplace(
        key,
        std::move(texture)
    );

    return *it->second;
}

const CubeMap &ResourceManager::loadCubeMap(
    const std::string& key,
    std::span<const std::string> faces
) {
    if (const auto it = cubeMaps.find(key); it != cubeMaps.end()) {
        return *it->second;
    }

    std::vector<std::string> resolvedFaces;
    resolvedFaces.reserve(faces.size());
    for (const std::string& face : faces) {
        resolvedFaces.push_back(resolvePath(face).lexically_normal().string());
    }

    auto cubeMap = std::make_unique<CubeMap>(resolvedFaces);
    auto [it, inserted] = cubeMaps.emplace(key, std::move(cubeMap));
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

    return *it->second;
}
