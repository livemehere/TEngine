#pragma once

#include <array>
#include <optional>

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "../scene/Scene.h"
#include "RenderExtent.h"
#include "RenderQueue.h"
#include "RenderSettings.h"
#include "RenderStats.h"
#include "GBuffer.h"
#include "FrameBufferDebugRenderer.h"
#include "SSAOProcessor.h"
#include "PointShadowMap.h"
#include "ShadowMap.h"
#include "mesh/MeshRendererComponent.h"

class ResourceManager;
class Shader;
class CubeMap;

constexpr std::size_t MAX_POINT_LIGHTS = 16;
constexpr std::size_t MAX_DIRECTIONAL_LIGHTS = 4;
constexpr std::size_t MAX_SPOT_LIGHTS = 8;

namespace UniformBinding {
    constexpr GLuint Camera = 0;
    constexpr GLuint Lights = 1;
    constexpr GLuint Debug = 2;
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

struct alignas(16) GPUDebugData {
    int viewMode;
    float depthNear;
    float depthFar;
    int padding;
};

static_assert(sizeof(GPUDebugData) == 16);

struct RenderOptions {
    std::optional<EntityId> highlightedEntityId;
};

class Renderer {
    const Shader &phongShader;
    const Shader &pbrShader;
    const Shader &outlineShader;
    const Shader &skyboxShader;
    const Shader &normalDebugShader;
    const Shader &deferredGeometryShader;
    const Shader &deferredLightingShader;
    const Shader &shadowDepthShader;
    const Shader &pointShadowDepthShader;
    ShadowMap shadowMap;
    PointShadowMap pointShadowMap;
    GBuffer gBuffer;
    SSAOProcessor ssaoProcessor;
    FrameBufferDebugRenderer frameBufferDebugRenderer;
    glm::vec4 outlineColor{ 0.4f, 0.8f, 0.0f, 1.0f};
    float outlineWidth = 0.02f;
    glm::vec4 normalDebugColor{1.0f, 0.75f, 0.1f, 1.0f};

    GLuint cameraUBO = 0;
    GLuint lightsUBO = 0;
    GLuint debugUBO = 0;
    GLuint instanceVBO = 0;
    GLuint fullscreenVAO = 0;
    GLuint currentTargetFramebuffer = 0;
    RenderExtent currentRenderExtent{1, 1};
    std::size_t instanceBufferCapacity = 0;
    RenderSettings currentSettings;
    RenderStats currentStats;
    const CubeMap* currentEnvironmentMap = nullptr;
    glm::mat4 currentLightSpaceMatrix{1.0f};
    glm::vec3 currentShadowLightDirection{0.0f, -1.0f, 0.0f};
    int currentShadowLightIndex = -1;
    bool currentShadowAvailable = false;
    std::array<glm::mat4, 6> currentPointShadowMatrices{};
    glm::vec3 currentPointShadowLightPosition{0.0f};
    float currentPointShadowFarPlane = 1.0f;
    int currentPointShadowLightIndex = -1;
    bool currentPointShadowAvailable = false;
    const FrameBuffer* currentSSAOTexture = nullptr;
    float currentCameraNear = 0.1f;
    float currentCameraFar = 1000.0f;
    bool currentCameraOrthographic = false;

    void updateCameraBuffer(const Scene& scene, const RenderExtent& size);
    void updateLightsBuffer(const Scene& scene);
    void updateDebugBuffer();
    void updateDirectionalShadow(const Scene& scene);
    void bindDirectionalShadow(const Shader& shader);
    void updatePointShadow();
    void bindPointShadow(const Shader& shader);

    /** passes */
    [[nodiscard]] RenderQueue buildRenderQueue(const Scene& scene, const RenderOptions& options) const;
    void opaqueRenderPass(const RenderQueue& queue, bool skipDeferredItems);
    void instancedOpaqueRenderPass(
        const RenderQueue& queue,
        bool skipDeferredItems
    );
    void deferredGeometryPass(const RenderQueue& queue);
    void deferredLightingPass(const FrameBuffer* ssaoTexture);
    void frameBufferDebugPass();
    void transparentRenderPass(const RenderQueue& queue);
    void normalDebugRenderPass(const RenderQueue& queue);
    void outlineRenderPass(const RenderQueue& queue);
    void shadowDepthRenderPass(const RenderQueue& queue);
    void pointShadowDepthRenderPass(const RenderQueue& queue);
    void drawShadowCasters(
        const RenderQueue& queue,
        const Shader& shader,
        std::uint64_t triangleMultiplier
    );
    void meshRenderPass(const glm::mat4& worldMatrix, const Mesh& mesh, const Material& material, bool writeOutlineStencil);
    void skyboxRenderPass();
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
        if (debugUBO != 0) {
            glDeleteBuffers(1, &debugUBO);
        }
        if (instanceVBO != 0) {
            glDeleteBuffers(1, &instanceVBO);
        }
        if (fullscreenVAO != 0) {
            glDeleteVertexArrays(1, &fullscreenVAO);
        }
    }
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void beginFrame(
        const Scene& scene,
        const RenderExtent& size,
        const RenderSettings& settings
    );
    void render(const Scene& scene, const RenderOptions& options = {});
    void endFrame();
    [[nodiscard]] const RenderStats& getStats() const { return currentStats; }
    [[nodiscard]] GLuint getFrameBufferDebugTextureId() const {
        return frameBufferDebugRenderer.getTextureId();
    }
};
