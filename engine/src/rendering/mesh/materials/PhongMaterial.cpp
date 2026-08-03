#include "PhongMaterial.h"

#include "../../../graphics/CubeMap.h"

namespace {
    constexpr GLuint EnvironmentTextureSlot = 2;
}

void PhongMaterial::bind() const {
    shader.use();

    // this material
    albedoTexture.bind(0);
    shader.setInt("material.albedo",0);
    shader.setVec4("material.baseColor",baseColor);
    shader.setFloat("material.shininess",shininess);
    shader.setFloat("material.specularStrength",specularStrength);
    shader.setFloat("material.environmentReflectivity", environmentReflectivity);

    if (specularTexture) {
        specularTexture->bind(1);
        shader.setInt("material.specular",1);
        shader.setInt("material.hasSpecularMap",1);
    }else {
        shader.setInt("material.hasSpecularMap",0);
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
