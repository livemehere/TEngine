#pragma once

#include "Material.h"
#include "../../../graphics/Texture2D.h"


class LitMaterial : public Material {
public:
    const Texture2D& albedoTexture;
    const Texture2D* specularTexture = nullptr;
    glm::vec4 baseColor;
    float shininess;
    float specularStrength;

    LitMaterial(
        const Shader &shader,
        const Texture2D& albedoTexture,
        glm::vec4 baseColor = glm::vec4(1.0f),
        float shininess = 32.0f,
        float specularStrength = 1.0f
    ) : Material(shader),
        albedoTexture(albedoTexture),
        baseColor(baseColor),
        shininess(shininess),
        specularStrength(specularStrength) {}

    ~LitMaterial() override = default;

    void bind() const override;
};
