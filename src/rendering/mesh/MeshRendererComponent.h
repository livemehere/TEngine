#pragma once

#include "Mesh.h"
#include "materials/Material.h"

enum class OutlineMode : int {
    NormalExtrusion = 0,
    ScaleFromPivot = 1
};

struct MeshRendererComponent {
    const Mesh* mesh = nullptr;
    const Material* material = nullptr;
    bool outlineEnabled = false;
    OutlineMode outlineMode = OutlineMode::NormalExtrusion;
};
