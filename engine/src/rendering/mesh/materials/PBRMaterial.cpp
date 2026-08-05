#include "PBRMaterial.h"

#include <algorithm>

namespace {
    constexpr GLuint AlbedoTextureSlot = 0;
    constexpr GLuint NormalTextureSlot = 5;
    // Renderer reserves 3 and 4 for directional and point shadow maps.
    constexpr GLuint MetallicTextureSlot = 6;
    constexpr GLuint RoughnessTextureSlot = 7;
    constexpr GLuint AOTextureSlot = 8;
}

void PBRMaterial::bindTo(const Shader& targetShader) const {
    targetShader.use();

    albedoTexture.bind(AlbedoTextureSlot);
    targetShader.setInt("material.albedo", AlbedoTextureSlot);
    targetShader.setVec4("material.baseColor", baseColor);
    targetShader.setFloat("material.metallic", std::clamp(metallic, 0.0f, 1.0f));
    targetShader.setFloat("material.roughness", std::clamp(roughness, 0.04f, 1.0f));
    targetShader.setFloat("material.ao", std::clamp(ao, 0.0f, 1.0f));

    const bool hasNormalMap = normalTexture && useNormalMapping;
    targetShader.setInt("material.hasNormalMap", hasNormalMap ? 1 : 0);
    targetShader.setFloat("material.normalStrength", normalStrength);
    targetShader.setInt("material.flipNormalY", flipNormalY ? 1 : 0);
    if (hasNormalMap) {
        normalTexture->bind(NormalTextureSlot);
        targetShader.setInt("material.normalMap", NormalTextureSlot);
    }

    targetShader.setInt("material.hasMetallicMap", metallicTexture ? 1 : 0);
    targetShader.setInt("material.metallicChannel", std::clamp(metallicChannel, 0, 3));
    if (metallicTexture) {
        metallicTexture->bind(MetallicTextureSlot);
        targetShader.setInt("material.metallicMap", MetallicTextureSlot);
    }

    targetShader.setInt("material.hasRoughnessMap", roughnessTexture ? 1 : 0);
    targetShader.setInt("material.roughnessChannel", std::clamp(roughnessChannel, 0, 3));
    if (roughnessTexture) {
        roughnessTexture->bind(RoughnessTextureSlot);
        targetShader.setInt("material.roughnessMap", RoughnessTextureSlot);
    }

    targetShader.setInt("material.hasAOMap", aoTexture ? 1 : 0);
    targetShader.setInt("material.aoChannel", std::clamp(aoChannel, 0, 3));
    if (aoTexture) {
        aoTexture->bind(AOTextureSlot);
        targetShader.setInt("material.aoMap", AOTextureSlot);
    }
}

void PBRMaterial::bind() const {
    bindTo(shader);
}

bool PBRMaterial::supportsDeferred() const {
    return renderQueue == RenderQueueType::Opaque;
}

void PBRMaterial::bindGeometry(const Shader& targetShader) const {
    bindTo(targetShader);
    targetShader.setInt("material.workflow", 1);
    targetShader.setInt("material.hasSpecularMap", 0);
    targetShader.setInt("material.hasDepthMap", 0);
    targetShader.setInt("material.parallaxMode", 0);
    targetShader.setInt("material.discardParallaxEdges", 0);
    targetShader.setInt("material.useBlinnPhong", 0);
    targetShader.setFloat("material.shininess", 1.0f);
    targetShader.setFloat("material.specularStrength", 0.0f);
}
