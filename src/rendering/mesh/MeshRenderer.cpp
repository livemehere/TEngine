#include "MeshRenderer.h"

#include "materials/UnlitMaterial.h"

void MeshRenderer::render(const MeshRenderObject &object) const {
    /* stencil */
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glStencilFunc(
        GL_ALWAYS,
        1,
        0xFF
    );

    glStencilOp(
    GL_KEEP,
    GL_KEEP,
    GL_REPLACE
    );

    /* material specific */
    object.material->bind();

    /* mesh common */
    const glm::mat4 model = object.transform.getModelMatrix();
    object.material->shader.setMat4("uModel", model);

    object.mesh->draw();


    /* 2. Outline path */
    glStencilFunc(
        GL_NOTEQUAL,
        1,
        0xFF
    );

    // glStencilMask(0x00);
    glDisable(GL_DEPTH_TEST);

    static std::array<uint8_t,4> pixels = {
        255,255,255,255,
    };
    static Texture2D whiteTexture{1,1,pixels};
    static Shader unlitShader{"shaders/basic.vert", "shaders/unlit.frag"};
    static UnlitMaterial unlitMaterial{unlitShader,whiteTexture};
    unlitMaterial.baseColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);

    Transform outlineTransform = object.transform;
    outlineTransform.scale *= 1.02f;
    auto outlineModel = outlineTransform.getModelMatrix();
    unlitMaterial.bind();
    unlitMaterial.shader.setMat4("uModel", outlineModel);
    object.mesh->draw();

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
}
