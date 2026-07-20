#pragma once

#include "Material.h"
#include "../../../graphics/Texture2D.h"


class UnlitMaterial : public Material {
public:
    Texture2D& albedoTexture;
    glm::vec4 baseColor;

    UnlitMaterial(
        Shader &shader,
        Texture2D& albedoTexture,
        glm::vec4 baseColor = glm::vec4(1.0f)
    ) : Material(shader),
        albedoTexture(albedoTexture),
        baseColor(baseColor) {}
    ~UnlitMaterial() override = default;

    void bind() const override;
};