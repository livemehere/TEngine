#pragma once
#include "MeshRendererComponent.h"

class MeshRenderer {
public:
    MeshRenderer() = default;
    ~MeshRenderer() = default;

    void render(const glm::mat4& worldMatrix, const Mesh& mesh, const Material& material) const;
};
