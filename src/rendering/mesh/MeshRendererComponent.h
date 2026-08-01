#pragma once

#include "Mesh.h"
#include "materials/Material.h"

struct MeshRendererComponent {
    const Mesh* mesh = nullptr;
    const Material* material = nullptr;
    // TODO: add outline (stencil) bool option
};

