#pragma once
#include "../Scene.h"
#include "mesh/MeshRenderer.h"

constexpr std::size_t MAX_POINT_LIGHTS = 16;

namespace UniformBinding {
    constexpr GLuint Camera = 0;
    constexpr GLuint Lights = 1;
}

struct alignas(16) GPUCameraData {
   glm::mat4 viewMatrix;
   glm::mat4 projectionMatrix;
   glm::vec4 position; // xyz
};

struct alignas(16) GPUPointLight {
    glm::vec4 positionRange;
    glm::vec4 colorIntensity;
};

struct alignas(16) GPULightingData {
    // rgb : color
    // w : intensity
    glm::vec4 ambientLightColorIntensity;

    // x: directionalLight / y : pointLight
    glm::ivec4 lightCounts;
    std::array<GPUPointLight,MAX_POINT_LIGHTS> pointLights;
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
