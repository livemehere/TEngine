#pragma once

#include "Material.h"
#include "../../../graphics/Texture2D.h"

enum class EnvironmentMappingMode : int {
    Reflection = 0,
    Refraction = 1
};

enum class ParallaxMappingMode : int {
    Disabled = 0,
    Basic = 1,
    Steep = 2,
    Occlusion = 3
};

class PhongMaterial : public Material {
public:
    const Texture2D& albedoTexture;
    const Texture2D* specularTexture = nullptr;
    const Texture2D* normalTexture = nullptr;
    const Texture2D* depthTexture = nullptr;
    glm::vec4 baseColor;
    float shininess;
    float specularStrength;
    bool useBlinnPhong = false;
    bool useNormalMapping = true;
    bool flipNormalY = false;
    float normalStrength = 1.0f;
    ParallaxMappingMode parallaxMappingMode =
            ParallaxMappingMode::Disabled;
    float parallaxScale = 0.05f;
    int parallaxMinLayers = 8;
    int parallaxMaxLayers = 32;
    bool discardParallaxEdges = false;
    EnvironmentMappingMode environmentMappingMode =
            EnvironmentMappingMode::Reflection;
    float environmentStrength = 0.0f;
    float refractiveIndex = 1.52f;

    PhongMaterial(
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

    ~PhongMaterial() override = default;

    void bind() const override;
    [[nodiscard]] bool supportsDeferred() const override;
    void bindGeometry(const Shader& targetShader) const override;
    void bindEnvironment(const CubeMap* environmentMap) const override;
};
