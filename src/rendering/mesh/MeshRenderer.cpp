#include "MeshRenderer.h"

#include <glm/gtx/transform.hpp>

#include "materials/UnlitMaterial.h"

void MeshRenderer::render(const glm::mat4& worldMatrix, const Mesh& mesh, const Material& material) const {
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
    material.shader.setMat4("uModel", worldMatrix);

    mesh.draw();


    /* 2. Outline path */
    glStencilFunc(
        GL_NOTEQUAL,
        1,
        0xFF
    );
    glStencilMask(0x00);
    glDisable(GL_DEPTH_TEST);

    // TODO: move to renderobject options
    static std::array<uint8_t,4> pixels = {
        255,255,255,255,
    };
    static Texture2D whiteTexture{1,1,pixels};
    static Shader unlitShader{"shaders/basic.vert", "shaders/unlit.frag"};
    static UnlitMaterial unlitMaterial{unlitShader,whiteTexture};
    unlitMaterial.baseColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);

    glm::mat4 outlineMatrix = glm::scale(worldMatrix, glm::vec3(1.02f));
    unlitMaterial.bind();
    unlitMaterial.shader.setMat4("uModel", outlineMatrix);
    mesh.draw();

    glStencilMask(0xFF);
    glEnable(GL_DEPTH_TEST);
}
