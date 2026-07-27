#include "MeshRenderer.h"

#include "materials/UnlitMaterial.h"

void MeshRenderer::render(const Transform& transform, const Mesh& mesh, const Material& material) const {
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
    material.bind();

    /* mesh common */
    const glm::mat4 model = transform.getModelMatrix();
    material.shader.setMat4("uModel", model);

    mesh.draw();


    /* 2. Outline path */
    glStencilFunc(
        GL_NOTEQUAL,
        1,
        0xFF
    );

    // glStencilMask(0x00);
    glDisable(GL_DEPTH_TEST);

    // TODO: move to renderobject options
    static std::array<uint8_t,4> pixels = {
        255,255,255,255,
    };
    static Texture2D whiteTexture{1,1,pixels};
    static Shader unlitShader{"shaders/basic.vert", "shaders/unlit.frag"};
    static UnlitMaterial unlitMaterial{unlitShader,whiteTexture};
    unlitMaterial.baseColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);

    Transform outlineTransform = transform;
    outlineTransform.scale *= 1.02f;
    auto outlineModel = outlineTransform.getModelMatrix();
    unlitMaterial.bind();
    unlitMaterial.shader.setMat4("uModel", outlineModel);
    mesh.draw();

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
}
