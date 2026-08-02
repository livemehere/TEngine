#include "Renderer.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>

#include "../resources/ResourceManager.h"
#include "../graphics/CubeMap.h"
#include "Lights.h"
#include "skybox/SkyboxComponent.h"

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
    : outlineShader(resourceManager.getOutlineShader()),
      skyboxShader(resourceManager.getSkyboxShader()) {
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
}

void Renderer::updateCameraBuffer(const Scene &scene, const RenderExtent& size) {
    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    GPUCameraData data{
        .viewMatrix = glm::mat4(1.0f),
        .projectionMatrix = glm::mat4(1.0f),
        .position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
    };

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
    }

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
            GPUDirectionalLLight &destination = data.directionalLights[directionalLightCount++];
            destination.colorIntensity = glm::vec4(light.color, light.intensity);
            destination.direction = glm::vec4(getWorldForward(scene, entity), 0.0f);
        }
    );

    scene.each<TransformComponent, PointLightComponent>(
        [&](const Entity &entity,
            const TransformComponent &,
            const PointLightComponent &light) {
            if (!light.enabled || pointLightCount >= MAX_POINT_LIGHTS) {
                return;
            }
            GPUPointLight &destination = data.pointLights[pointLightCount++];
            destination.colorIntensity = glm::vec4(light.color, light.intensity);
            destination.positionRange = glm::vec4(getWorldPosition(scene, entity), light.range);
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


void Renderer::beginFrame(const Scene &scene, const RenderExtent& size) {
    updateCameraBuffer(scene, size);
    updateLightsBuffer(scene);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_PROGRAM_POINT_SIZE);
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

    /* mesh common */
    material.shader.setMat4("uModel", worldMatrix);

    mesh.draw();
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
}

void Renderer::skyboxRenderPass(const Scene &scene) {
    const CubeMap* cubeMap = nullptr;
    scene.each<SkyboxComponent>([&](const Entity &, const SkyboxComponent &skybox) {
        if (!cubeMap && skybox.enabled && skybox.cubeMap) {
            cubeMap = skybox.cubeMap;
        }
    });

    if (!cubeMap) {
        return;
    }

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    skyboxShader.use();
    cubeMap->bind(0);
    cubeMap->draw();

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
}

void Renderer::render(const Scene &scene, const RenderOptions &options) {
    const auto isHighlighted = [&](const Entity &entity) {
        return options.highlightedEntityId &&
               isEntityOrDescendantOf(
                   scene,
                   entity,
                   *options.highlightedEntityId
               );
    };

    const auto getOutlineVisibility = [&](const Entity &entity)
        -> std::optional<OutlineVisibility> {
        const MeshRendererComponent* component =
                entity.tryGetComponent<MeshRendererComponent>();

        if (!component || !component->enabled || !component->mesh || !component->material) {
            return std::nullopt;
        }

        if (isHighlighted(entity)) {
            return OutlineVisibility::AlwaysVisible;
        }

        if (!component->outlineEnabled) {
            return std::nullopt;
        }

        return component->outlineVisibility;
    };

    struct XRayOutlineGroup {
        EntityId id;
        std::vector<const Entity *> entities;
    };

    bool hasVisibleOutline = false;
    std::vector<XRayOutlineGroup> xRayOutlineGroups;

    // Establish the background first without writing depth. Scene meshes can
    // then overwrite it normally and populate the depth buffer.
    skyboxRenderPass(scene);

    // Scene mesh pass: finish the depth buffer and mark visible-only outlines.
    scene.each<TransformComponent, MeshRendererComponent>(
        [&](const Entity &entity,
            const TransformComponent &,
            const MeshRendererComponent &component) {
            if (!component.enabled || !component.mesh || !component.material) {
                return;
            }

            const std::optional<OutlineVisibility> visibility = getOutlineVisibility(entity);
            const bool visibleOnly = visibility == OutlineVisibility::VisibleOnly;

            const glm::mat4 worldMatrix = scene.getWorldMatrix(entity);
            meshRenderPass(worldMatrix, *component.mesh, *component.material, visibleOnly);

            hasVisibleOutline |= visibleOnly;

            if (visibility == OutlineVisibility::AlwaysVisible) {
                const EntityId groupId = isHighlighted(entity)
                                             ? *options.highlightedEntityId
                                             : getTopLevelAncestorId(scene, entity);

                auto group = std::ranges::find(
                    xRayOutlineGroups,
                    groupId,
                    &XRayOutlineGroup::id
                );

                if (group == xRayOutlineGroups.end()) {
                    group = xRayOutlineGroups.insert(
                        xRayOutlineGroups.end(),
                        XRayOutlineGroup{.id = groupId}
                    );
                }

                group->entities.push_back(&entity);
            }
        }
    );

    if (hasVisibleOutline) {
        // Visible-only outline pass: use the completed scene depth buffer.
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        scene.each<TransformComponent, MeshRendererComponent>(
            [&](const Entity &entity,
                const TransformComponent &,
                const MeshRendererComponent &component) {
                if (!component.enabled ||
                    !component.mesh ||
                    !component.material ||
                    getOutlineVisibility(entity) != OutlineVisibility::VisibleOnly) {
                    return;
                }

                const glm::mat4 worldMatrix = scene.getWorldMatrix(entity);
                drawMeshOutline(worldMatrix, *component.mesh, component.outlineMode, outlineWidth);
            }
        );

        glDepthMask(GL_TRUE);
        glStencilMask(0xFF);
        glDisable(GL_STENCIL_TEST);
    }

    if (!xRayOutlineGroups.empty()) {
        glEnable(GL_STENCIL_TEST);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        for (const XRayOutlineGroup &group : xRayOutlineGroups) {
            // Each group needs its own stencil mask. Otherwise a large selected
            // object can suppress the X-ray outline of an unrelated object.
            glStencilMask(0xFF);
            glClear(GL_STENCIL_BUFFER_BIT);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

            for (const Entity *entity : group.entities) {
                const MeshRendererComponent &component =
                        entity->getComponent<MeshRendererComponent>();
                const glm::mat4 worldMatrix = scene.getWorldMatrix(*entity);
                drawMeshOutline(worldMatrix, *component.mesh, component.outlineMode, 0.0f);
            }

            // Draw only outside this group's original silhouette, ignoring depth.
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
            glStencilMask(0x00);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

            for (const Entity *entity : group.entities) {
                const MeshRendererComponent &component =
                        entity->getComponent<MeshRendererComponent>();
                const glm::mat4 worldMatrix = scene.getWorldMatrix(*entity);
                drawMeshOutline(worldMatrix, *component.mesh, component.outlineMode, outlineWidth);
            }
        }

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glStencilMask(0xFF);
        glDisable(GL_STENCIL_TEST);
    }
}

void Renderer::endFrame() {
}
