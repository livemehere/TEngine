#pragma once

#include "Material.h"
#include "../../../graphics/Texture2D.h"


class UnlitMaterial : public Material {
public:
    const Texture2D& albedoTexture;
    glm::vec4 baseColor;

    UnlitMaterial(
        const Shader &shader,
        const Texture2D& albedoTexture,
        glm::vec4 baseColor = glm::vec4(1.0f)
    ) : Material(shader),
        albedoTexture(albedoTexture),
        baseColor(baseColor) {}
    ~UnlitMaterial() override = default;

    void bind() const override;
};