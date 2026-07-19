#include "MeshRenderer.h"

#include "../Renderer.h"
#include "materials/BasicMaterial.h"

void MeshRenderer::render(const MeshRenderObject &object) const {
    /* material specific */
    object.material->bind();

    /* all shader global */
    object.material->shader.bindUniformBlock("CameraData", UniformBinding::Camera);
    object.material->shader.bindUniformBlock("LightsData", UniformBinding::Lights);

    /* mesh common */
    const glm::mat4 model = object.transform.getModelMatrix();
    object.material->shader.setMat4("uModel", model);

    object.mesh->draw();
}
