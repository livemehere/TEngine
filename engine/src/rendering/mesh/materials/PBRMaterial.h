#pragma once

#include "Material.h"
#include "../../../graphics/Texture2D.h"

class PBRMaterial final : public Material {
public:
    const Texture2D& albedoTexture;
    const Texture2D* normalTexture = nullptr;
    const Texture2D* metallicTexture = nullptr;
    const Texture2D* roughnessTexture = nullptr;
    const Texture2D* aoTexture = nullptr;

    glm::vec4 baseColor{1.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
    bool useNormalMapping = true;
    bool flipNormalY = false;
    float normalStrength = 1.0f;

    // Separate scalar maps normally use R. glTF packs roughness into G and
    // metallic into B, so imported materials can select those channels.
    int metallicChannel = 0;
    int roughnessChannel = 0;
    int aoChannel = 0;

    PBRMaterial(
        const Shader& shader,
        const Texture2D& albedoTexture
    ) : Material(shader),
        albedoTexture(albedoTexture) {}

    ~PBRMaterial() override = default;

    void bind() const override;
    [[nodiscard]] bool supportsDeferred() const override;
    void bindGeometry(const Shader& targetShader) const override;

private:
    void bindTo(const Shader& targetShader) const;
};
