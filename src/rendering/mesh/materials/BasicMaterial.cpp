#include "BasicMaterial.h"

void BasicMaterial::bind(const RenderContext& context) {
    shader.use();

    // texture
    texture.bind(0);
    shader.setInt("uTexture",0);

    shader.setVec4("uColor",color);

    shader.setMat4("uView",context.view);
    shader.setMat4("uProjection",context.projection);
}
