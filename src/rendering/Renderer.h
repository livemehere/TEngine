#pragma once

#include <optional>

#include <glm/vec4.hpp>

#include "../scene/Scene.h"
#include "RenderExtent.h"

class ResourceManager;
class Shader;

constexpr std::size_t MAX_POINT_LIGHTS = 16;
constexpr std::size_t MAX_DIRECTIONAL_LIGHTS = 4;
constexpr std::size_t MAX_SPOT_LIGHTS = 8;

namespace UniformBinding {
    constexpr GLuint Camera = 0;
    constexpr GLuint Lights = 1;
}

struct alignas(16) GPUCameraData {
   glm::mat4 viewMatrix;
   glm::mat4 projectionMatrix;
   glm::vec4 position; // xyz
};

struct alignas(16) GPUDirectionalLLight {
    glm::vec4 direction;
    glm::vec4 colorIntensity;
};

struct alignas(16) GPUPointLight {
    glm::vec4 positionRange;
    glm::vec4 colorIntensity;
};

struct alignas(16) GPUSpotLight {
    glm::vec4 direction;
    glm::vec4 positionRange;
    glm::vec4 colorIntensity;

    // x: cos(innerAngle)
    // y: cos(outerAngle)
    glm::vec4 coneAngles;
};

struct alignas(16) GPULightingData {
    // rgb : color
    // w : intensity
    glm::vec4 ambientLightColorIntensity;

    // x: directionalLight / y : pointLight / z : spotLight
    glm::ivec4 lightCounts;
    std::array<GPUDirectionalLLight,MAX_DIRECTIONAL_LIGHTS> directionalLights;
    std::array<GPUPointLight,MAX_POINT_LIGHTS> pointLights;
    std::array<GPUSpotLight,MAX_SPOT_LIGHTS> spotLights;
};

struct RenderOptions {
    std::optional<EntityId> highlightedEntityId;
};

class Renderer {
    const Shader &outlineShader;
    glm::vec4 outlineColor{ 0.4f, 0.8f, 0.0f, 1.0f};
    float outlineWidth = 0.02f;

    GLuint cameraUBO = 0;
    GLuint lightsUBO = 0;

    void updateCameraBuffer(Scene& scene, const RenderExtent& size);
    void updateLightsBuffer(Scene& scene);

    /** passes */
    void meshRenderPass(const glm::mat4& worldMatrix, const Mesh& mesh, const Material& material, bool writeOutlineStencil);
    void drawMeshOutline(const glm::mat4& worldMatrix, const Mesh& mesh, OutlineMode outlineMode, float width);

public:
    explicit Renderer(ResourceManager &resourceManager);
    ~Renderer() {
        if (cameraUBO != 0) {
            glDeleteBuffers(1, &cameraUBO);
        }
        if (lightsUBO != 0) {
            glDeleteBuffers(1, &lightsUBO);
        }
    }
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void beginFrame(Scene& scene, const RenderExtent& size);
    void render(const Scene& scene, const RenderOptions& options = {});
    void endFrame();
};
