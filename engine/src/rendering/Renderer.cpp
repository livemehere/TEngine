#include "Renderer.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat3x3.hpp>

#include "../resources/ResourceManager.h"
#include "../graphics/CubeMap.h"
#include "Lights.h"
#include "skybox/SkyboxComponent.h"
#include "model/InstancedModelRendererComponent.h"

namespace {
    void applyCullMode(CullMode mode) {
        switch (mode) {
            case CullMode::None:
                glDisable(GL_CULL_FACE);
                break;
            case CullMode::Back:
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                break;
            case CullMode::Front:
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);
                break;
        }
    }

    bool isEntityOrDescendantOf(
        const Scene &scene,
        const Entity &entity,
        EntityId ancestorId
    ) {
        const Entity *current = &entity;

        while (current) {
            if (current->id == ancestorId) {
                return true;
            }

            current = current->parentId
                          ? scene.findEntity(*current->parentId)
                          : nullptr;
        }

        return false;
    }

    EntityId getTopLevelAncestorId(const Scene &scene, const Entity &entity) {
        const Entity *current = &entity;

        while (current->parentId) {
            const Entity *parent = scene.findEntity(*current->parentId);
            if (!parent) {
                break;
            }
            current = parent;
        }

        return current->id;
    }

    glm::vec3 getWorldPosition(const Scene &scene, const Entity &entity) {
        return glm::vec3(scene.getWorldMatrix(entity)[3]);
    }

    glm::vec3 getWorldForward(const Scene &scene, const Entity &entity) {
        const glm::vec3 forward = glm::mat3(scene.getWorldMatrix(entity)) *
                                  glm::vec3(0.0f, 0.0f, -1.0f);
        const float lengthSquared = glm::dot(forward, forward);
        if (lengthSquared <= 1e-8f) {
            return {0.0f, 0.0f, -1.0f};
        }
        return forward / std::sqrt(lengthSquared);
    }

    const Material* getDeferredMaterial(const Material* material) {
        if (!material || !material->supportsDeferred()) {
            return nullptr;
        }
        return material;
    }
}

/*layout (std140) uniform ExampleBlock
{
                     // base alignment  // aligned offset
    float value;     // 4               // 0
    vec3 vector;     // 16              // 16  (offset must be multiple of 16 so 4->16)
    mat4 matrix;     // 16              // 32  (column 0)
                     // 16              // 48  (column 1)
                     // 16              // 64  (column 2)
                     // 16              // 80  (column 3)
    float values[3]; // 16              // 96  (values[0])
                     // 16              // 112 (values[1])
                     // 16              // 128 (values[2])
    bool boolean;    // 4               // 144
    int integer;     // 4               // 148
}; */

Renderer::Renderer(ResourceManager &resourceManager)
    : phongShader(resourceManager.getPhongShader()),
      pbrShader(resourceManager.getPBRShader()),
      outlineShader(resourceManager.getOutlineShader()),
      skyboxShader(resourceManager.getSkyboxShader()),
      normalDebugShader(resourceManager.getNormalDebugShader()),
      deferredGeometryShader(resourceManager.getDeferredGeometryShader()),
      deferredLightingShader(resourceManager.getDeferredLightingShader()),
      shadowDepthShader(resourceManager.getShadowDepthShader()),
      pointShadowDepthShader(resourceManager.getPointShadowDepthShader()),
      shadowMap(1),
      pointShadowMap(1),
      gBuffer({1, 1}),
      ssaoProcessor(resourceManager),
      frameBufferDebugRenderer(resourceManager),
      textRenderer(resourceManager) {
    /* camera */
    glGenBuffers(1, &cameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GPUCameraData), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, UniformBinding::Camera, cameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    /* lights */
    glGenBuffers(1, &lightsUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GPULightingData), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, UniformBinding::Lights, lightsUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    /* debug view */
    glGenBuffers(1, &debugUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, debugUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GPUDebugData), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, UniformBinding::Debug, debugUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    /* per-instance model matrices */
    glGenBuffers(1, &instanceVBO);

    glGenVertexArrays(1, &fullscreenVAO);

    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    deferredLightingShader.use();
    deferredLightingShader.setInt("gPosition", 0);
    deferredLightingShader.setInt("gNormal", 1);
    deferredLightingShader.setInt("gAlbedoSpec", 2);
    deferredLightingShader.setInt("gMaterial", 6);
    deferredLightingShader.setInt("uSSAO", 5);
    glUseProgram(previousProgram);
}

void Renderer::updateCameraBuffer(const Scene &scene, const RenderExtent& size) {
    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    GPUCameraData data{
        .viewMatrix = glm::mat4(1.0f),
        .projectionMatrix = glm::mat4(1.0f),
        .position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
    };
    currentCameraNear = 0.1f;
    currentCameraFar = 1000.0f;
    currentCameraOrthographic = false;

    if (const Entity *cameraEntity = scene.getActiveCameraEntity()) {
        const CameraComponent &camera = cameraEntity->getComponent<CameraComponent>();
        const glm::mat4 worldMatrix = scene.getWorldMatrix(*cameraEntity);

        Transform cameraWorldTransform;
        if (Transform::decompose(worldMatrix, cameraWorldTransform)) {
            cameraWorldTransform.scale = {1.0f, 1.0f, 1.0f};
            data.viewMatrix = glm::inverse(cameraWorldTransform.getLocalMatrix());
        } else {
            data.viewMatrix = glm::inverse(worldMatrix);
        }

        data.projectionMatrix = camera.getProjectionMatrix(size);
        data.position = glm::vec4(glm::vec3(worldMatrix[3]), 1.0f);

        if (std::holds_alternative<OrthoGraphicProjection>(camera.projection)) {
            const auto &projection =
                    std::get<OrthoGraphicProjection>(camera.projection);
            currentCameraNear = projection.near;
            currentCameraFar = projection.far;
            currentCameraOrthographic = true;
        } else {
            const auto &projection =
                    std::get<PerspectiveProjection>(camera.projection);
            currentCameraNear = projection.near;
            currentCameraFar = projection.far;
        }
    }

    currentViewMatrix = data.viewMatrix;
    currentProjectionMatrix = data.projectionMatrix;

    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GPUCameraData), &data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Renderer::updateLightsBuffer(const Scene &scene) {
    // TODO: upload light to GPU, only closed to camera for performance
    glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
    GPULightingData data{};

    glm::vec3 ambientColor{0.0f};
    size_t directionalLightCount = 0;
    size_t pointLightCount = 0;
    size_t spotLightCount = 0;
    currentShadowLightIndex = -1;
    currentPointShadowLightIndex = -1;

    scene.each<AmbientLightComponent>(
        [&](const AmbientLightComponent &light) {
            if (light.enabled) {
                ambientColor += light.color * light.intensity;
            }
        }
    );

    scene.each<TransformComponent, DirectionalLightComponent>(
        [&](const Entity &entity,
            const TransformComponent &,
            const DirectionalLightComponent &light) {
            if (!light.enabled || directionalLightCount >= MAX_DIRECTIONAL_LIGHTS) {
                return;
            }
            const int lightIndex = static_cast<int>(directionalLightCount);
            GPUDirectionalLLight &destination =
                    data.directionalLights[directionalLightCount++];
            destination.colorIntensity = glm::vec4(light.color, light.intensity);
            destination.direction = glm::vec4(getWorldForward(scene, entity), 0.0f);

            if (currentShadowLightIndex < 0 && light.castShadows) {
                currentShadowLightIndex = lightIndex;
                currentShadowLightDirection = glm::vec3(destination.direction);
            }
        }
    );

    scene.each<TransformComponent, PointLightComponent>(
        [&](const Entity &entity,
            const TransformComponent &,
            const PointLightComponent &light) {
            if (!light.enabled || pointLightCount >= MAX_POINT_LIGHTS) {
                return;
            }
            const int lightIndex = static_cast<int>(pointLightCount);
            GPUPointLight &destination = data.pointLights[pointLightCount++];
            destination.colorIntensity = glm::vec4(light.color, light.intensity);
            destination.positionRange = glm::vec4(getWorldPosition(scene, entity), light.range);

            if (currentPointShadowLightIndex < 0 && light.castShadows) {
                currentPointShadowLightIndex = lightIndex;
                currentPointShadowLightPosition =
                        glm::vec3(destination.positionRange);
                currentPointShadowFarPlane = light.range;
            }
        }
    );

    scene.each<TransformComponent, SpotLightComponent>(
        [&](const Entity &entity,
            const TransformComponent &,
            const SpotLightComponent &light) {
            if (!light.enabled || spotLightCount >= MAX_SPOT_LIGHTS) {
                return;
            }
            GPUSpotLight &destination = data.spotLights[spotLightCount++];
            destination.direction = glm::vec4(getWorldForward(scene, entity), 0.0f);
            destination.colorIntensity = glm::vec4(light.color, light.intensity);
            destination.positionRange = glm::vec4(getWorldPosition(scene, entity), light.range);
            destination.coneAngles = glm::vec4(
                std::cos(glm::radians(light.innerAngle)),
                std::cos(glm::radians(light.outerAngle)),
                0.0f,
                0.0f
            );
        }
    );

    data.ambientLightColorIntensity = glm::vec4(ambientColor, 1.0f);
    data.lightCounts = glm::ivec4(
        static_cast<int>(directionalLightCount),
        static_cast<int>(pointLightCount),
        static_cast<int>(spotLightCount),
        0
    );

    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GPULightingData), &data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Renderer::updateDirectionalShadow(const Scene &scene) {
    const bool needsDirectionalShadow =
            currentSettings.debugView == DebugViewMode::Shaded ||
            currentSettings.frameBufferDebugView ==
                FrameBufferDebugView::DirectionalShadow;
    currentShadowAvailable =
            currentSettings.shadowsEnabled &&
            needsDirectionalShadow &&
            currentShadowLightIndex >= 0;
    if (!currentShadowAvailable) {
        return;
    }

    const float halfExtent = std::max(currentSettings.shadowDistance, 0.1f);
    glm::vec3 center{0.0f};

    if (const Entity *cameraEntity = scene.getActiveCameraEntity()) {
        center = getWorldPosition(scene, *cameraEntity);
        center += getWorldForward(scene, *cameraEntity) * (halfExtent * 0.25f);
    }

    const glm::vec3 direction = glm::normalize(currentShadowLightDirection);
    const glm::vec3 lightPosition = center - direction * halfExtent;
    const glm::vec3 worldUp{0.0f, 1.0f, 0.0f};
    const glm::vec3 viewUp = std::abs(glm::dot(direction, worldUp)) > 0.99f
                                 ? glm::vec3{0.0f, 0.0f, 1.0f}
                                 : worldUp;

    const glm::mat4 lightProjection = glm::ortho(
        -halfExtent,
        halfExtent,
        -halfExtent,
        halfExtent,
        0.1f,
        halfExtent * 2.0f
    );
    const glm::mat4 lightView = glm::lookAt(
        lightPosition,
        center,
        viewUp
    );
    currentLightSpaceMatrix = lightProjection * lightView;
}

void Renderer::bindDirectionalShadow(const Shader& shader) {
    constexpr GLuint ShadowTextureSlot = 3;

    shader.use();
    shader.setInt(
        "uShadowsEnabled",
        currentShadowAvailable ? 1 : 0
    );
    shader.setInt("uShadowLightIndex", currentShadowLightIndex);
    shader.setMat4("uLightSpaceMatrix", currentLightSpaceMatrix);
    shader.setFloat("uShadowBiasMin", currentSettings.shadowBiasMin);
    shader.setFloat("uShadowBiasSlope", currentSettings.shadowBiasSlope);
    shader.setInt("uShadowPcfRadius", currentSettings.shadowPcfRadius);
    shader.setInt("uShadowMap", ShadowTextureSlot);
    shadowMap.bindTexture(ShadowTextureSlot);
}

void Renderer::updatePointShadow() {
    constexpr float NearPlane = 0.1f;
    const bool needsPointShadow =
            currentSettings.debugView == DebugViewMode::Shaded ||
            currentSettings.frameBufferDebugView ==
                FrameBufferDebugView::PointShadow;
    currentPointShadowAvailable =
            currentSettings.pointShadowsEnabled &&
            needsPointShadow &&
            currentPointShadowLightIndex >= 0 &&
            currentPointShadowFarPlane > NearPlane;
    if (!currentPointShadowAvailable) {
        return;
    }

    const glm::mat4 projection = glm::perspective(
        glm::radians(90.0f),
        1.0f,
        NearPlane,
        currentPointShadowFarPlane
    );
    const glm::vec3 &position = currentPointShadowLightPosition;
    currentPointShadowMatrices = {
        projection * glm::lookAt(
            position,
            position + glm::vec3{1.0f, 0.0f, 0.0f},
            glm::vec3{0.0f, -1.0f, 0.0f}
        ),
        projection * glm::lookAt(
            position,
            position + glm::vec3{-1.0f, 0.0f, 0.0f},
            glm::vec3{0.0f, -1.0f, 0.0f}
        ),
        projection * glm::lookAt(
            position,
            position + glm::vec3{0.0f, 1.0f, 0.0f},
            glm::vec3{0.0f, 0.0f, 1.0f}
        ),
        projection * glm::lookAt(
            position,
            position + glm::vec3{0.0f, -1.0f, 0.0f},
            glm::vec3{0.0f, 0.0f, -1.0f}
        ),
        projection * glm::lookAt(
            position,
            position + glm::vec3{0.0f, 0.0f, 1.0f},
            glm::vec3{0.0f, -1.0f, 0.0f}
        ),
        projection * glm::lookAt(
            position,
            position + glm::vec3{0.0f, 0.0f, -1.0f},
            glm::vec3{0.0f, -1.0f, 0.0f}
        )
    };
}

void Renderer::bindPointShadow(const Shader& shader) {
    constexpr GLuint PointShadowTextureSlot = 4;

    shader.use();
    shader.setInt(
        "uPointShadowsEnabled",
        currentPointShadowAvailable ? 1 : 0
    );
    shader.setInt(
        "uPointShadowLightIndex",
        currentPointShadowLightIndex
    );
    shader.setVec3(
        "uPointShadowLightPosition",
        currentPointShadowLightPosition
    );
    shader.setFloat(
        "uPointShadowFarPlane",
        currentPointShadowFarPlane
    );
    shader.setFloat(
        "uPointShadowBias",
        currentSettings.pointShadowBias
    );
    shader.setFloat(
        "uPointShadowSoftness",
        currentSettings.pointShadowSoftness
    );
    shader.setInt(
        "uPointShadowSampleCount",
        currentSettings.pointShadowSampleCount
    );
    shader.setInt("uPointShadowMap", PointShadowTextureSlot);
    pointShadowMap.bindTexture(PointShadowTextureSlot);
}

void Renderer::updateDebugBuffer() {
    const GPUDebugData data{
        .viewMode = static_cast<int>(currentSettings.debugView),
        .depthNear = currentSettings.debugDepthNear,
        .depthFar = currentSettings.debugDepthFar,
        .padding = 0
    };

    glBindBuffer(GL_UNIFORM_BUFFER, debugUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GPUDebugData), &data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


void Renderer::beginFrame(
    const Scene &scene,
    const RenderExtent& size,
    const RenderSettings& settings
) {
    currentSettings = settings;
    currentStats = {};
    currentSSAOTexture = nullptr;
    frameBufferDebugRenderer.reset();
    currentRenderExtent = size;
    GLint targetFramebuffer = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &targetFramebuffer);
    currentTargetFramebuffer = static_cast<GLuint>(targetFramebuffer);
    if (currentSettings.renderingPath == RenderingPath::Deferred) {
        gBuffer.resize(size);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, currentTargetFramebuffer);
    glViewport(0, 0, size.width, size.height);
    updateCameraBuffer(scene, size);
    updateLightsBuffer(scene);
    updateDebugBuffer();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_PROGRAM_POINT_SIZE);
    if (currentSettings.renderingPath == RenderingPath::Forward &&
        currentSettings.msaaSamples > 1) {
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
    }
    glFrontFace(GL_CCW);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // render as wireframe
    // glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // glClear respects the stencil write mask. Always restore all stencil bits
    // before clearing so values from the previous frame cannot survive.
    glStencilMask(0xFF);
    glDisable(GL_STENCIL_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Renderer::meshRenderPass(const glm::mat4 &worldMatrix, const Mesh &mesh, const Material &material,
                              bool writeOutlineStencil) {
    if (writeOutlineStencil) {
        glEnable(GL_STENCIL_TEST);
        glStencilMask(0xFF);
        glStencilFunc(
            GL_ALWAYS,
            1,
            0xFF
        );

        glStencilOp(
            GL_KEEP,
            GL_KEEP,
            GL_REPLACE
        );
    } else {
        glDisable(GL_STENCIL_TEST);
    }

    /* material specific */
    applyCullMode(material.rasterState.cullMode);
    material.bind();
    material.bindEnvironment(currentEnvironmentMap);

    /* mesh common */
    material.shader.setMat4("uModel", worldMatrix);

    mesh.draw();
    ++currentStats.drawCalls;
    currentStats.triangleCount += mesh.getTriangleCount();
}

void Renderer::drawMeshOutline(
    const glm::mat4 &worldMatrix,
    const Mesh &mesh,
    OutlineMode outlineMode,
    float width
) {
    // Outline geometry owns its raster state and must not inherit the material's
    // culling mode. Both sides are required by the current stencil extrusion.
    glDisable(GL_CULL_FACE);

    outlineShader.use();
    outlineShader.setMat4("uModel", worldMatrix);
    outlineShader.setFloat("uOutlineWidth", width);
    outlineShader.setVec4("uOutlineColor", outlineColor);
    outlineShader.setInt("uOutlineMode", static_cast<int>(outlineMode));
    mesh.draw();
    ++currentStats.drawCalls;
    currentStats.triangleCount += mesh.getTriangleCount();
}

void Renderer::skyboxRenderPass() {
    if (!currentEnvironmentMap) {
        return;
    }

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    skyboxShader.use();
    currentEnvironmentMap->bind(0);
    currentEnvironmentMap->draw();
    ++currentStats.drawCalls;
    currentStats.triangleCount += 12;

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
}

RenderQueue Renderer::buildRenderQueue(
    const Scene &scene,
    const RenderOptions &options
) const {
    RenderQueue queue;

    glm::vec3 cameraPosition{0.0f};
    if (const Entity *cameraEntity = scene.getActiveCameraEntity()) {
        cameraPosition = glm::vec3(scene.getWorldMatrix(*cameraEntity)[3]);
    }

    const auto isHighlighted = [&](const Entity &entity) {
        return options.highlightedEntityId &&
               isEntityOrDescendantOf(
                   scene,
                   entity,
                   *options.highlightedEntityId
               );
    };

    const auto getOutlineVisibility = [&](
        const Entity &entity,
        const MeshRendererComponent &component
    )
        -> std::optional<OutlineVisibility> {
        if (isHighlighted(entity)) {
            return OutlineVisibility::AlwaysVisible;
        }

        if (!component.outlineEnabled) {
            return std::nullopt;
        }

        return component.outlineVisibility;
    };

    scene.each<TransformComponent, MeshRendererComponent>(
        [&](const Entity &entity,
            const TransformComponent &,
            const MeshRendererComponent &component) {
            if (!component.enabled || !component.mesh || !component.material) {
                return;
            }

            const glm::mat4 worldMatrix = scene.getWorldMatrix(entity);
            const glm::vec3 offset = glm::vec3(worldMatrix[3]) - cameraPosition;

            RenderItem item{
                .entity = &entity,
                .meshRenderer = &component,
                .worldMatrix = worldMatrix,
                .cameraDistanceSquared = glm::dot(offset, offset),
                .outlineVisibility = getOutlineVisibility(entity, component)
            };

            if (component.showVertexNormals) {
                queue.normalDebug.push_back(item);
            }

            switch (component.material->renderQueue) {
                case RenderQueueType::Opaque:
                    queue.opaque.push_back(std::move(item));
                    break;
                case RenderQueueType::AlphaCutout:
                    queue.alphaCutout.push_back(std::move(item));
                    break;
                case RenderQueueType::Transparent:
                    queue.transparent.push_back(std::move(item));
                    break;
            }
        }
    );

    scene.each<TransformComponent, InstancedMeshRendererComponent>(
        [&](const Entity &entity,
            const TransformComponent &,
            const InstancedMeshRendererComponent &component) {
            if (!component.enabled ||
                !component.mesh ||
                !component.material ||
                component.instances.empty() ||
                component.material->renderQueue != RenderQueueType::Opaque) {
                return;
            }

            InstancedRenderItem item{
                .mesh = component.mesh,
                .material = component.material,
                .worldMatrix = scene.getWorldMatrix(entity)
            };
            item.localMatrices.reserve(component.instances.size());
            for (const InstanceData &instance : component.instances.getItems()) {
                item.localMatrices.push_back(instance.transform.getLocalMatrix());
            }
            queue.instancedOpaque.push_back(std::move(item));
        }
    );

    scene.each<TransformComponent, InstancedModelRendererComponent>(
        [&](const Entity &entity,
            const TransformComponent &,
            const InstancedModelRendererComponent &component) {
            if (!component.enabled || !component.model || component.instances.empty()) {
                return;
            }

            const Model &model = *component.model;
            std::vector<glm::mat4> nodeMatrices(model.nodes.size(), glm::mat4{1.0f});

            for (std::size_t nodeIndex = 0; nodeIndex < model.nodes.size(); ++nodeIndex) {
                const ModelNode &node = model.nodes[nodeIndex];
                const glm::mat4 localMatrix = node.localTransform.getLocalMatrix();
                nodeMatrices[nodeIndex] = node.parentIndex
                    ? nodeMatrices[*node.parentIndex] * localMatrix
                    : localMatrix;

                for (const std::size_t partIndex : node.partIndices) {
                    if (partIndex >= model.parts.size()) {
                        continue;
                    }

                    const ModelPart &part = model.parts[partIndex];
                    if (!part.mesh) {
                        continue;
                    }

                    const Material *material = component.fallbackMaterial;
                    if (part.materialSlot < model.materials.size() &&
                        model.materials[part.materialSlot]) {
                        material = model.materials[part.materialSlot];
                    }
                    if (!material || material->renderQueue != RenderQueueType::Opaque) {
                        continue;
                    }

                    InstancedRenderItem item{
                        .mesh = part.mesh.get(),
                        .material = material,
                        .worldMatrix = scene.getWorldMatrix(entity)
                    };
                    item.localMatrices.reserve(component.instances.size());
                    for (const InstanceData &instance : component.instances.getItems()) {
                        item.localMatrices.push_back(
                            instance.transform.getLocalMatrix() * nodeMatrices[nodeIndex]
                        );
                    }
                    queue.instancedOpaque.push_back(std::move(item));
                }
            }
        }
    );

    const auto sortByRenderState = [](const RenderItem &left, const RenderItem &right) {
        const Material *leftMaterial = left.meshRenderer->material;
        const Material *rightMaterial = right.meshRenderer->material;

        if (&leftMaterial->shader != &rightMaterial->shader) {
            return std::less<const Shader *>{}(
                &leftMaterial->shader,
                &rightMaterial->shader
            );
        }
        if (leftMaterial != rightMaterial) {
            return std::less<const Material *>{}(leftMaterial, rightMaterial);
        }
        return std::less<const Mesh *>{}(
            left.meshRenderer->mesh,
            right.meshRenderer->mesh
        );
    };

    std::sort(queue.opaque.begin(), queue.opaque.end(), sortByRenderState);
    std::sort(queue.alphaCutout.begin(), queue.alphaCutout.end(), sortByRenderState);
    std::sort(
        queue.instancedOpaque.begin(),
        queue.instancedOpaque.end(),
        [](const InstancedRenderItem &left, const InstancedRenderItem &right) {
            const Material *leftMaterial = left.material;
            const Material *rightMaterial = right.material;

            if (&leftMaterial->shader != &rightMaterial->shader) {
                return std::less<const Shader *>{}(
                    &leftMaterial->shader,
                    &rightMaterial->shader
                );
            }
            if (leftMaterial != rightMaterial) {
                return std::less<const Material *>{}(leftMaterial, rightMaterial);
            }
            return std::less<const Mesh *>{}(
                left.mesh,
                right.mesh
            );
        }
    );
    std::stable_sort(
        queue.transparent.begin(),
        queue.transparent.end(),
        [](const RenderItem &left, const RenderItem &right) {
            return left.cameraDistanceSquared > right.cameraDistanceSquared;
        }
    );

    const auto collectOutline = [&](const RenderItem &item) {
        if (!item.outlineVisibility) {
            return;
        }

        if (*item.outlineVisibility == OutlineVisibility::VisibleOnly) {
            queue.visibleOutlines.push_back(item);
            return;
        }

        const EntityId groupId = isHighlighted(*item.entity)
                                     ? *options.highlightedEntityId
                                     : getTopLevelAncestorId(scene, *item.entity);

        auto group = std::ranges::find(
            queue.xRayOutlineGroups,
            groupId,
            &XRayOutlineGroup::id
        );

        if (group == queue.xRayOutlineGroups.end()) {
            group = queue.xRayOutlineGroups.insert(
                queue.xRayOutlineGroups.end(),
                XRayOutlineGroup{.id = groupId}
            );
        }

        group->items.push_back(item);
    };

    for (const RenderItem &item : queue.opaque) {
        collectOutline(item);
    }
    for (const RenderItem &item : queue.alphaCutout) {
        collectOutline(item);
    }
    for (const RenderItem &item : queue.transparent) {
        collectOutline(item);
    }

    return queue;
}

void Renderer::opaqueRenderPass(
    const RenderQueue &queue,
    const bool skipDeferredItems
) {
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glPolygonMode(
        GL_FRONT_AND_BACK,
        currentSettings.rasterization == RasterizationMode::Wireframe
            ? GL_LINE
            : GL_FILL
    );

    const auto drawItem = [&](const RenderItem &item) {
        if (skipDeferredItems &&
            getDeferredMaterial(item.meshRenderer->material)) {
            return;
        }
        meshRenderPass(
            item.worldMatrix,
            *item.meshRenderer->mesh,
            *item.meshRenderer->material,
            item.outlineVisibility == OutlineVisibility::VisibleOnly
        );
    };

    for (const RenderItem &item : queue.opaque) {
        drawItem(item);
    }
    for (const RenderItem &item : queue.alphaCutout) {
        drawItem(item);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::transparentRenderPass(const RenderQueue &queue) {
    if (queue.transparent.empty()) {
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_FALSE);
    glPolygonMode(
        GL_FRONT_AND_BACK,
        currentSettings.rasterization == RasterizationMode::Wireframe
            ? GL_LINE
            : GL_FILL
    );

    for (const RenderItem &item : queue.transparent) {
        meshRenderPass(
            item.worldMatrix,
            *item.meshRenderer->mesh,
            *item.meshRenderer->material,
            item.outlineVisibility == OutlineVisibility::VisibleOnly
        );
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::instancedOpaqueRenderPass(
    const RenderQueue &queue,
    const bool skipDeferredItems
) {
    if (queue.instancedOpaque.empty()) {
        return;
    }

    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glPolygonMode(
        GL_FRONT_AND_BACK,
        currentSettings.rasterization == RasterizationMode::Wireframe
            ? GL_LINE
            : GL_FILL
    );

    for (const InstancedRenderItem &item : queue.instancedOpaque) {
        if (skipDeferredItems && getDeferredMaterial(item.material)) {
            continue;
        }
        const std::size_t instanceCount = item.localMatrices.size();
        const std::size_t byteSize = instanceCount * sizeof(glm::mat4);

        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        if (byteSize > instanceBufferCapacity) {
            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(byteSize),
                nullptr,
                GL_DYNAMIC_DRAW
            );
            instanceBufferCapacity = byteSize;
        }
        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            static_cast<GLsizeiptr>(byteSize),
            item.localMatrices.data()
        );

        applyCullMode(item.material->rasterState.cullMode);
        item.material->bind();
        item.material->bindEnvironment(currentEnvironmentMap);
        item.material->shader.setMat4("uModel", item.worldMatrix);
        item.mesh->drawInstanced(
            instanceVBO,
            static_cast<GLsizei>(instanceCount)
        );

        ++currentStats.drawCalls;
        ++currentStats.instancedDrawCalls;
        currentStats.instanceCount += instanceCount;
        currentStats.triangleCount +=
                static_cast<std::uint64_t>(item.mesh->getTriangleCount()) *
                instanceCount;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::deferredGeometryPass(const RenderQueue& queue) {
    gBuffer.bindForGeometry();
    gBuffer.clear();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glPolygonMode(
        GL_FRONT_AND_BACK,
        currentSettings.rasterization == RasterizationMode::Wireframe
            ? GL_LINE
            : GL_FILL
    );

    for (const RenderItem& item : queue.opaque) {
        const Material* material =
                getDeferredMaterial(item.meshRenderer->material);
        if (!material) {
            continue;
        }

        const bool writeOutlineStencil =
                item.outlineVisibility == OutlineVisibility::VisibleOnly;
        if (writeOutlineStencil) {
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        } else {
            glDisable(GL_STENCIL_TEST);
        }

        applyCullMode(material->rasterState.cullMode);
        material->bindGeometry(deferredGeometryShader);
        deferredGeometryShader.setMat4("uModel", item.worldMatrix);
        item.meshRenderer->mesh->draw();

        ++currentStats.drawCalls;
        ++currentStats.deferredGeometryDrawCalls;
        currentStats.triangleCount +=
                item.meshRenderer->mesh->getTriangleCount();
    }

    glDisable(GL_STENCIL_TEST);
    for (const InstancedRenderItem& item : queue.instancedOpaque) {
        const Material* material = getDeferredMaterial(item.material);
        if (!material) {
            continue;
        }

        const std::size_t instanceCount = item.localMatrices.size();
        const std::size_t byteSize = instanceCount * sizeof(glm::mat4);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        if (byteSize > instanceBufferCapacity) {
            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(byteSize),
                nullptr,
                GL_DYNAMIC_DRAW
            );
            instanceBufferCapacity = byteSize;
        }
        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            static_cast<GLsizeiptr>(byteSize),
            item.localMatrices.data()
        );

        applyCullMode(material->rasterState.cullMode);
        material->bindGeometry(deferredGeometryShader);
        deferredGeometryShader.setMat4("uModel", item.worldMatrix);
        item.mesh->drawInstanced(
            instanceVBO,
            static_cast<GLsizei>(instanceCount)
        );

        ++currentStats.drawCalls;
        ++currentStats.deferredGeometryDrawCalls;
        ++currentStats.instancedDrawCalls;
        currentStats.instanceCount += instanceCount;
        currentStats.triangleCount +=
                static_cast<std::uint64_t>(item.mesh->getTriangleCount()) *
                instanceCount;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::deferredLightingPass(const FrameBuffer* ssaoTexture) {
    glBindFramebuffer(GL_FRAMEBUFFER, currentTargetFramebuffer);
    glViewport(
        0,
        0,
        currentRenderExtent.width,
        currentRenderExtent.height
    );
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    deferredLightingShader.use();
    bindDirectionalShadow(deferredLightingShader);
    bindPointShadow(deferredLightingShader);
    gBuffer.bindTextures(0, 1, 2, 6);
    deferredLightingShader.setInt(
        "uSSAOEnabled",
        ssaoTexture != nullptr ? 1 : 0
    );
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(
        GL_TEXTURE_2D,
        ssaoTexture != nullptr
            ? ssaoTexture->getTextureId()
            : gBuffer.getPositionTextureId()
    );
    glBindVertexArray(fullscreenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    ++currentStats.drawCalls;
    ++currentStats.deferredLightingDrawCalls;

    glDepthMask(GL_TRUE);
    gBuffer.blitDepthStencilTo(
        currentTargetFramebuffer,
        currentRenderExtent
    );
    glBindFramebuffer(GL_FRAMEBUFFER, currentTargetFramebuffer);
    glViewport(
        0,
        0,
        currentRenderExtent.width,
        currentRenderExtent.height
    );
}

void Renderer::frameBufferDebugPass() {
    const FrameBufferDebugView mode = currentSettings.frameBufferDebugView;
    if (mode == FrameBufferDebugView::Off) {
        return;
    }

    const bool deferredSource =
            mode >= FrameBufferDebugView::GBufferPosition &&
            mode <= FrameBufferDebugView::SSAOBlurred;
    if (deferredSource &&
        currentSettings.renderingPath != RenderingPath::Deferred) {
        return;
    }

    // Keep valid textures bound to both sampler types. Some macOS OpenGL
    // drivers validate every active sampler even when its branch is unused.
    GLuint texture2D = gBuffer.getPositionTextureId();
    GLuint cubeTexture = pointShadowMap.getTextureId();
    switch (mode) {
        case FrameBufferDebugView::Off:
            return;
        case FrameBufferDebugView::GBufferPosition:
            texture2D = gBuffer.getPositionTextureId();
            break;
        case FrameBufferDebugView::GBufferNormal:
            texture2D = gBuffer.getNormalTextureId();
            break;
        case FrameBufferDebugView::GBufferAlbedo:
        case FrameBufferDebugView::GBufferSpecular:
            texture2D = gBuffer.getAlbedoSpecTextureId();
            break;
        case FrameBufferDebugView::GBufferMetallic:
        case FrameBufferDebugView::GBufferRoughness:
        case FrameBufferDebugView::GBufferAO:
            texture2D = gBuffer.getMaterialTextureId();
            break;
        case FrameBufferDebugView::GBufferDepth:
            texture2D = gBuffer.getDepthStencilTextureId();
            break;
        case FrameBufferDebugView::SSAORaw:
            if (!currentSSAOTexture) {
                return;
            }
            texture2D = ssaoProcessor.getRawTextureId();
            break;
        case FrameBufferDebugView::SSAOBlurred:
            if (!currentSSAOTexture) {
                return;
            }
            texture2D = ssaoProcessor.getBlurredTextureId();
            break;
        case FrameBufferDebugView::DirectionalShadow:
            if (!currentShadowAvailable) {
                return;
            }
            texture2D = shadowMap.getTextureId();
            break;
        case FrameBufferDebugView::PointShadow:
            if (!currentPointShadowAvailable) {
                return;
            }
            cubeTexture = pointShadowMap.getTextureId();
            break;
    }

    if (frameBufferDebugRenderer.render({
        .mode = mode,
        .texture2D = texture2D,
        .cubeTexture = cubeTexture,
        .extent = currentRenderExtent,
        .cameraNear = currentCameraNear,
        .cameraFar = currentCameraFar,
        .depthRangeNear = currentSettings.debugDepthNear,
        .depthRangeFar = currentSettings.debugDepthFar,
        .orthographic = currentCameraOrthographic,
        .cubeFace = currentSettings.pointShadowDebugFace
    })) {
        ++currentStats.drawCalls;
        ++currentStats.frameBufferDebugDrawCalls;
    }
}

void Renderer::normalDebugRenderPass(const RenderQueue &queue) {
    if (queue.normalDebug.empty()) {
        return;
    }

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    normalDebugShader.use();
    normalDebugShader.setVec4("uColor", normalDebugColor);

    for (const RenderItem &item : queue.normalDebug) {
        normalDebugShader.setMat4("uModel", item.worldMatrix);
        normalDebugShader.setFloat(
            "uNormalLength",
            item.meshRenderer->vertexNormalLength
        );
        item.meshRenderer->mesh->draw();
        ++currentStats.drawCalls;
        currentStats.triangleCount +=
                item.meshRenderer->mesh->getTriangleCount();
    }

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

void Renderer::outlineRenderPass(const RenderQueue &queue) {
    if (!queue.visibleOutlines.empty()) {
        // Visible-only outline pass: use the completed scene depth buffer.
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        for (const RenderItem &item : queue.visibleOutlines) {
            drawMeshOutline(
                item.worldMatrix,
                *item.meshRenderer->mesh,
                item.meshRenderer->outlineMode,
                outlineWidth
            );
        }

        glDepthMask(GL_TRUE);
        glStencilMask(0xFF);
        glDisable(GL_STENCIL_TEST);
    }

    if (!queue.xRayOutlineGroups.empty()) {
        glEnable(GL_STENCIL_TEST);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        for (const XRayOutlineGroup &group : queue.xRayOutlineGroups) {
            // Each group needs its own stencil mask. Otherwise a large selected
            // object can suppress the X-ray outline of an unrelated object.
            glStencilMask(0xFF);
            glClear(GL_STENCIL_BUFFER_BIT);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

            for (const RenderItem &item : group.items) {
                drawMeshOutline(
                    item.worldMatrix,
                    *item.meshRenderer->mesh,
                    item.meshRenderer->outlineMode,
                    0.0f
                );
            }

            // Draw only outside this group's original silhouette, ignoring depth.
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
            glStencilMask(0x00);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

            for (const RenderItem &item : group.items) {
                drawMeshOutline(
                    item.worldMatrix,
                    *item.meshRenderer->mesh,
                    item.meshRenderer->outlineMode,
                    outlineWidth
                );
            }
        }

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glStencilMask(0xFF);
        glDisable(GL_STENCIL_TEST);
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void Renderer::shadowDepthRenderPass(const RenderQueue &queue) {
    if (!currentShadowAvailable) {
        return;
    }

    shadowMap.resize(currentSettings.shadowMapResolution);

    GLint previousDrawFramebuffer = 0;
    GLint previousReadFramebuffer = 0;
    GLint previousViewport[4]{};
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    shadowMap.bindForWriting();
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glClear(GL_DEPTH_BUFFER_BIT);

    shadowDepthShader.use();
    shadowDepthShader.setMat4(
        "uLightSpaceMatrix",
        currentLightSpaceMatrix
    );

    drawShadowCasters(queue, shadowDepthShader, 1);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFramebuffer);
    glViewport(
        previousViewport[0],
        previousViewport[1],
        previousViewport[2],
        previousViewport[3]
    );
}

void Renderer::pointShadowDepthRenderPass(const RenderQueue &queue) {
    if (!currentPointShadowAvailable) {
        return;
    }

    pointShadowMap.resize(currentSettings.pointShadowMapResolution);

    GLint previousDrawFramebuffer = 0;
    GLint previousReadFramebuffer = 0;
    GLint previousViewport[4]{};
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    pointShadowMap.bindForWriting();
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glClear(GL_DEPTH_BUFFER_BIT);

    pointShadowDepthShader.use();
    for (std::size_t face = 0;
         face < currentPointShadowMatrices.size();
         ++face) {
        const std::string uniformName =
                "uShadowMatrices[" + std::to_string(face) + "]";
        pointShadowDepthShader.setMat4(
            uniformName.c_str(),
            currentPointShadowMatrices[face]
        );
    }
    pointShadowDepthShader.setVec3(
        "uLightPosition",
        currentPointShadowLightPosition
    );
    pointShadowDepthShader.setFloat(
        "uFarPlane",
        currentPointShadowFarPlane
    );

    drawShadowCasters(queue, pointShadowDepthShader, 6);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFramebuffer);
    glViewport(
        previousViewport[0],
        previousViewport[1],
        previousViewport[2],
        previousViewport[3]
    );
}

void Renderer::drawShadowCasters(
    const RenderQueue &queue,
    const Shader &shader,
    const std::uint64_t triangleMultiplier
) {

    for (const RenderItem &item : queue.opaque) {
        applyCullMode(item.meshRenderer->material->rasterState.cullMode);
        shader.setMat4("uModel", item.worldMatrix);
        item.meshRenderer->mesh->draw();

        ++currentStats.drawCalls;
        ++currentStats.shadowDrawCalls;
        const std::uint64_t triangleCount =
                static_cast<std::uint64_t>(
                    item.meshRenderer->mesh->getTriangleCount()
                ) * triangleMultiplier;
        currentStats.triangleCount += triangleCount;
        currentStats.shadowTriangleCount += triangleCount;
    }

    for (const InstancedRenderItem &item : queue.instancedOpaque) {
        const std::size_t instanceCount = item.localMatrices.size();
        const std::size_t byteSize = instanceCount * sizeof(glm::mat4);

        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        if (byteSize > instanceBufferCapacity) {
            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(byteSize),
                nullptr,
                GL_DYNAMIC_DRAW
            );
            instanceBufferCapacity = byteSize;
        }
        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            static_cast<GLsizeiptr>(byteSize),
            item.localMatrices.data()
        );

        applyCullMode(item.material->rasterState.cullMode);
        shader.setMat4("uModel", item.worldMatrix);
        item.mesh->drawInstanced(
            instanceVBO,
            static_cast<GLsizei>(instanceCount)
        );

        ++currentStats.drawCalls;
        ++currentStats.shadowDrawCalls;
        const std::uint64_t triangleCount =
                static_cast<std::uint64_t>(item.mesh->getTriangleCount()) *
                instanceCount *
                triangleMultiplier;
        currentStats.triangleCount += triangleCount;
        currentStats.shadowTriangleCount += triangleCount;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::render(const Scene &scene, const RenderOptions &options) {
    currentEnvironmentMap = nullptr;
    scene.each<SkyboxComponent>([&](const SkyboxComponent &skybox) {
        if (!currentEnvironmentMap && skybox.enabled && skybox.cubeMap) {
            currentEnvironmentMap = skybox.cubeMap;
        }
    });

    const RenderQueue queue = buildRenderQueue(scene, options);

    updateDirectionalShadow(scene);
    updatePointShadow();
    shadowDepthRenderPass(queue);
    pointShadowDepthRenderPass(queue);

    const bool useDeferred =
            currentSettings.renderingPath == RenderingPath::Deferred;
    if (useDeferred) {
        deferredGeometryPass(queue);
        const FrameBuffer* ssaoTexture = ssaoProcessor.process(
            gBuffer,
            currentSettings,
            currentRenderExtent
        );
        if (ssaoTexture) {
            currentStats.drawCalls += 2;
            currentStats.ssaoDrawCalls += 2;
        }
        currentSSAOTexture = ssaoTexture;
        deferredLightingPass(ssaoTexture);

        const bool isGBufferDebugView =
                currentSettings.debugView >=
                DebugViewMode::GBufferPosition;
        if (isGBufferDebugView) {
            return;
        }
    }

    bindDirectionalShadow(phongShader);
    bindPointShadow(phongShader);
    bindDirectionalShadow(pbrShader);
    bindPointShadow(pbrShader);

    // Deferred-compatible opaque Phong items are already present in the
    // target. The remaining special/transparent items use forward rendering
    // against the depth and stencil copied from the G-buffer.
    opaqueRenderPass(queue, useDeferred);
    instancedOpaqueRenderPass(queue, useDeferred);

    if (currentSettings.debugView == DebugViewMode::Shaded) {
        skyboxRenderPass();
    }
    transparentRenderPass(queue);
    normalDebugRenderPass(queue);
    if (currentSettings.rasterization == RasterizationMode::Fill) {
        outlineRenderPass(queue);
    }
    worldTextRenderPass(scene);
}

void Renderer::endFrame() {
    frameBufferDebugPass();
}

void Renderer::worldTextRenderPass(const Scene &scene) {
    if (textRenderer.renderWorld(
        scene,
        currentViewMatrix,
        currentProjectionMatrix
    )) {
        ++currentStats.drawCalls;
        ++currentStats.textDrawCalls;
    }
}

void Renderer::renderCanvas(const Scene &scene, const RenderExtent extent) {
    if (textRenderer.renderCanvas(scene, extent)) {
        ++currentStats.drawCalls;
        ++currentStats.textDrawCalls;
    }
}
