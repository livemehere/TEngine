#include "LitMaterial.h"

void LitMaterial::bind() const {
    shader.use();

    // this material
    albedoTexture.bind(0);
    shader.setInt("material.albedo",0);
    shader.setVec4("material.baseColor",baseColor);
    shader.setFloat("material.shininess",shininess);
    shader.setFloat("material.specularStrength",specularStrength);

    if (specularTexture) {
        specularTexture->bind(1);
        shader.setInt("material.specular",1);
        shader.setInt("material.hasSpecularMap",1);
    }else {
        shader.setInt("material.hasSpecularMap",0);
    }
}
