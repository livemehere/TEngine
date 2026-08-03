#pragma once

#include "../../scene/Component.h"
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

class MeshRendererComponent final : public Component {
public:
    const Mesh* mesh = nullptr;
    const Material* material = nullptr;
    bool outlineEnabled = false;
    OutlineMode outlineMode = OutlineMode::NormalExtrusion;
    OutlineVisibility outlineVisibility = OutlineVisibility::VisibleOnly;
    bool showVertexNormals = false;
    float vertexNormalLength = 0.2f;

    MeshRendererComponent() = default;

    MeshRendererComponent(const Mesh* mesh, const Material* material)
        : mesh(mesh), material(material) {}
};
