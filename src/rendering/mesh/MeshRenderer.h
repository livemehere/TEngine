#pragma once
#include "MeshRendererComponent.h"
#include "../Transform.h"

class MeshRenderer {
public:
    MeshRenderer() = default;
    ~MeshRenderer() = default;

    void render(const Transform& transform, const Mesh& mesh, const Material& material) const;
};
