#include "MeshRenderer.h"

#include "../Renderer.h"
#include "materials/LitMaterial.h"

void MeshRenderer::render(const MeshRenderObject &object) const {
    /* material specific */
    object.material->bind();

    /* mesh common */
    const glm::mat4 model = object.transform.getModelMatrix();
    object.material->shader.setMat4("uModel", model);

    object.mesh->draw();
}
