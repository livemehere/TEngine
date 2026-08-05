#include "PhongMaterial.h"

#include "../../../graphics/CubeMap.h"

namespace {
    constexpr GLuint EnvironmentTextureSlot = 2;
    // Renderer reserves 3 and 4 for directional and point shadow maps.
    constexpr GLuint NormalTextureSlot = 5;
    constexpr GLuint DepthTextureSlot = 6;
}

void PhongMaterial::bind() const {
    shader.use();

    // this material
    albedoTexture.bind(0);
    shader.setInt("material.albedo",0);
    shader.setVec4("material.baseColor",baseColor);
    shader.setFloat("material.shininess",shininess);
    shader.setFloat("material.specularStrength",specularStrength);
    shader.setInt("material.useBlinnPhong", useBlinnPhong ? 1 : 0);
    shader.setInt(
        "material.environmentMappingMode",
        static_cast<int>(environmentMappingMode)
    );
    shader.setFloat("material.environmentStrength", environmentStrength);
    shader.setFloat("material.refractiveIndex", refractiveIndex);

    if (specularTexture) {
        specularTexture->bind(1);
        shader.setInt("material.specular",1);
        shader.setInt("material.hasSpecularMap",1);
    }else {
        shader.setInt("material.hasSpecularMap",0);
    }

    const bool hasNormalMap = normalTexture && useNormalMapping;
    shader.setInt("material.hasNormalMap", hasNormalMap ? 1 : 0);
    shader.setFloat("material.normalStrength", normalStrength);
    shader.setInt("material.flipNormalY", flipNormalY ? 1 : 0);
    if (hasNormalMap) {
        normalTexture->bind(NormalTextureSlot);
        shader.setInt("material.normalMap", NormalTextureSlot);
    }

    const bool hasDepthMap =
            depthTexture &&
            parallaxMappingMode != ParallaxMappingMode::Disabled;
    shader.setInt("material.hasDepthMap", hasDepthMap ? 1 : 0);
    shader.setInt(
        "material.parallaxMode",
        static_cast<int>(parallaxMappingMode)
    );
    shader.setFloat("material.parallaxScale", parallaxScale);
    shader.setInt("material.parallaxMinLayers", parallaxMinLayers);
    shader.setInt("material.parallaxMaxLayers", parallaxMaxLayers);
    shader.setInt(
        "material.discardParallaxEdges",
        discardParallaxEdges ? 1 : 0
    );
    if (hasDepthMap) {
        depthTexture->bind(DepthTextureSlot);
        shader.setInt("material.depthMap", DepthTextureSlot);
    }
}

bool PhongMaterial::supportsDeferred() const {
    return renderQueue == RenderQueueType::Opaque &&
           environmentStrength <= 0.0f;
}

void PhongMaterial::bindGeometry(const Shader& targetShader) const {
    targetShader.use();

    albedoTexture.bind(0);
    targetShader.setInt("material.albedo", 0);
    targetShader.setVec4("material.baseColor", baseColor);
    targetShader.setFloat("material.shininess", shininess);
    targetShader.setFloat("material.specularStrength", specularStrength);
    targetShader.setInt(
        "material.useBlinnPhong",
        useBlinnPhong ? 1 : 0
    );
    targetShader.setInt("material.workflow", 0);

    if (specularTexture) {
        specularTexture->bind(1);
        targetShader.setInt("material.specular", 1);
        targetShader.setInt("material.hasSpecularMap", 1);
    } else {
        targetShader.setInt("material.hasSpecularMap", 0);
    }

    const bool hasNormalMap = normalTexture && useNormalMapping;
    targetShader.setInt("material.hasNormalMap", hasNormalMap ? 1 : 0);
    targetShader.setFloat("material.normalStrength", normalStrength);
    targetShader.setInt("material.flipNormalY", flipNormalY ? 1 : 0);
    if (hasNormalMap) {
        normalTexture->bind(NormalTextureSlot);
        targetShader.setInt("material.normalMap", NormalTextureSlot);
    }

    const bool hasDepthMap =
            depthTexture &&
            parallaxMappingMode != ParallaxMappingMode::Disabled;
    targetShader.setInt("material.hasDepthMap", hasDepthMap ? 1 : 0);
    targetShader.setInt(
        "material.parallaxMode",
        static_cast<int>(parallaxMappingMode)
    );
    targetShader.setFloat("material.parallaxScale", parallaxScale);
    targetShader.setInt("material.parallaxMinLayers", parallaxMinLayers);
    targetShader.setInt("material.parallaxMaxLayers", parallaxMaxLayers);
    targetShader.setInt(
        "material.discardParallaxEdges",
        discardParallaxEdges ? 1 : 0
    );
    if (hasDepthMap) {
        depthTexture->bind(DepthTextureSlot);
        targetShader.setInt("material.depthMap", DepthTextureSlot);
    }
}

void PhongMaterial::bindEnvironment(const CubeMap *environmentMap) const {
    shader.setInt("uHasEnvironmentMap", environmentMap ? 1 : 0);
    if (!environmentMap) {
        return;
    }

    environmentMap->bind(EnvironmentTextureSlot);
    shader.setInt("uEnvironmentMap", EnvironmentTextureSlot);
}
