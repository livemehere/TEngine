#include "PhongMaterial.h"

#include "../../../graphics/CubeMap.h"

namespace {
    constexpr GLuint EnvironmentTextureSlot = 2;
    // Renderer reserves 3 and 4 for directional and point shadow maps.
    constexpr GLuint NormalTextureSlot = 5;
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
}

void PhongMaterial::bindEnvironment(const CubeMap *environmentMap) const {
    shader.setInt("uHasEnvironmentMap", environmentMap ? 1 : 0);
    if (!environmentMap) {
        return;
    }

    environmentMap->bind(EnvironmentTextureSlot);
    shader.setInt("uEnvironmentMap", EnvironmentTextureSlot);
}
