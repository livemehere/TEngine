#pragma once

#include "../rendering/mesh/Mesh.h"
#include "../rendering/mesh/materials/Material.h"

struct MeshRendererComponent {
    const Mesh* mesh = nullptr;
    const Material* material = nullptr;
};

