#pragma once

#include "Mesh.h"
#include "materials/Material.h"

enum class OutlineMode : int {
    NormalExtrusion = 0,
    ScaleFromPivot = 1
};

enum class OutlineVisibility : int {
    VisibleOnly = 0,
    AlwaysVisible = 1
};

struct MeshRendererComponent {
    const Mesh* mesh = nullptr;
    const Material* material = nullptr;
    bool outlineEnabled = false;
    OutlineMode outlineMode = OutlineMode::NormalExtrusion;
    OutlineVisibility outlineVisibility = OutlineVisibility::VisibleOnly;
};
