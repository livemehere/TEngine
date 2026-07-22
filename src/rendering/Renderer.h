#pragma once
#include "../Scene.h"
#include "mesh/MeshRenderer.h"

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

class Renderer {
    MeshRenderer meshRenderer;

    GLuint cameraUBO = 0;
    GLuint lightsUBO = 0;

    void updateCameraBuffer(Scene& scene, const WindowSize& windowSize);
    void updateLightsBuffer(Scene& scene);
public:
    Renderer();
    ~Renderer() {
        if (cameraUBO != 0) {
            glDeleteBuffers(1, &cameraUBO);
        }
        if (lightsUBO != 0) {
            glDeleteBuffers(1, &lightsUBO);
        }
    }
    void beginFrame(Scene& scene, const WindowSize& windowSize);
    void render(const Scene& scene);
    void endFrame();
};
