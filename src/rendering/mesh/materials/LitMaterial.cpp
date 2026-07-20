#include "LitMaterial.h"

void LitMaterial::bind() const {
    shader.use();

    // this material
    albedoTexture.bind(0);
    shader.setInt("uAlbedoTexture",0);
    shader.setVec4("uBaseColor",baseColor);
    shader.setFloat("uShininess",shininess);
    shader.setFloat("uSpecularStrength",specularStrength);
}
