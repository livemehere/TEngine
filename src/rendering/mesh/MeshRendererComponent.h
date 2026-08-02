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

    MeshRendererComponent() = default;

    MeshRendererComponent(const Mesh* mesh, const Material* material)
        : mesh(mesh), material(material) {}

    [[nodiscard]] std::unique_ptr<Component> clone() const override {
        auto result = std::make_unique<MeshRendererComponent>(mesh, material);
        result->enabled = enabled;
        result->outlineEnabled = outlineEnabled;
        result->outlineMode = outlineMode;
        result->outlineVisibility = outlineVisibility;
        return result;
    }
};
