#pragma once

#include <vector>

#include <glm/mat4x4.hpp>

#include "../../scene/Component.h"
#include "Mesh.h"
#include "materials/Material.h"

class InstancedMeshRendererComponent final : public Component {
public:
    const Mesh *mesh = nullptr;
    const Material *material = nullptr;
    std::vector<glm::mat4> localMatrices;

    InstancedMeshRendererComponent() = default;

    InstancedMeshRendererComponent(
        const Mesh *mesh,
        const Material *material
    ) : mesh(mesh), material(material) {}
};
