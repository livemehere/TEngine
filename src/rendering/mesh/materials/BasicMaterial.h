#pragma once

#include "Material.h"
#include "../../../graphics/Texture2D.h"


class BasicMaterial : public Material {
public:
    glm::vec4 color;
    const Texture2D& texture;

    BasicMaterial(const Shader &shader, const Texture2D& texture, const glm::vec4 color = {1.0f,1.0f,1.0f,1.0f})
        : Material(shader), texture(texture), color(color) {}

    ~BasicMaterial() override = default;

    void bind(const RenderContext& context) override;
};
