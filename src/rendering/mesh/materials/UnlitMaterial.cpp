#include "UnlitMaterial.h"

void UnlitMaterial::bind() const {
    shader.use();

    // this material
    albedoTexture.bind(0);
    shader.setInt("uAlbedoTexture",0);
    shader.setVec4("uBaseColor",baseColor);
}
