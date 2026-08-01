#include "Renderer.h"

#include "../resources/ResourceManager.h"

namespace {
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
    : outlineShader(resourceManager.getOutlineShader()) {
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

void Renderer::updateCameraBuffer(Scene &scene, const WindowSize &windowSize) {
    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    const GPUCameraData data{
        .viewMatrix = scene.camera.getViewMatrix(),
        .projectionMatrix = scene.camera.getProjectionMatrix(windowSize),
        .position = glm::vec4(scene.camera.transform.position, 1.0f)
    };
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GPUCameraData), &data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Renderer::updateLightsBuffer(Scene &scene) {
    // TODO: upload light to GPU, only closed to camera for performance
    glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
    GPULightingData data{};
    data.ambientLightColorIntensity = glm::vec4(scene.ambientLight.color, scene.ambientLight.intensity);

    const auto directionalLightCount = static_cast<size_t>(std::min(scene.directionalLights.size(),
                                                                    MAX_DIRECTIONAL_LIGHTS));
    const auto pointLightCount = static_cast<size_t>(std::min(scene.pointLights.size(), MAX_POINT_LIGHTS));
    const auto spotLightCount = static_cast<size_t>(std::min(scene.spotLights.size(), MAX_SPOT_LIGHTS));

    data.lightCounts = glm::ivec4(
        directionalLightCount,
        pointLightCount,
        spotLightCount,
        0
    );

    for (int i = 0; i < directionalLightCount; i++) {
        const DirectionalLight &source = scene.directionalLights[i];
        data.directionalLights[i].colorIntensity = glm::vec4(source.color, source.intensity);
        data.directionalLights[i].direction = glm::vec4(source.direction, 0.0f);
    }

    for (int i = 0; i < pointLightCount; i++) {
        const PointLight &source = scene.pointLights[i];
        data.pointLights[i].colorIntensity = glm::vec4(source.color, source.intensity);
        data.pointLights[i].positionRange = glm::vec4(source.position, source.range);
    }

    for (int i = 0; i < spotLightCount; i++) {
        const SpotLight &source = scene.spotLights[i];
        data.spotLights[i].direction = glm::vec4(source.direction, 0.0f);
        data.spotLights[i].colorIntensity = glm::vec4(source.color, source.intensity);
        data.spotLights[i].positionRange = glm::vec4(source.position, source.range);
        data.spotLights[i].coneAngles = glm::vec4(
            std::cos(glm::radians(source.innerAngle)),
            std::cos(glm::radians(source.outerAngle)),
            0.0f,
            0.0f
        );
    }

    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GPULightingData), &data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


void Renderer::beginFrame(Scene &scene, const WindowSize &windowSize) {
    updateCameraBuffer(scene, windowSize);
    updateLightsBuffer(scene);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // render as wireframe
    // glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Back-face culling (enable for one-sided meshes)
    // NOTE: keep disabled when both sides of a surface must be visible.
    // glEnable(GL_CULL_FACE);
    // glFrontFace(GL_CCW);
    // glCullFace(GL_BACK);

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
    outlineShader.use();
    outlineShader.setMat4("uModel", worldMatrix);
    outlineShader.setFloat("uOutlineWidth", width);
    outlineShader.setVec4("uOutlineColor", outlineColor);
    outlineShader.setInt("uOutlineMode", static_cast<int>(outlineMode));
    mesh.draw();
}

void Renderer::render(const Scene &scene, const RenderOptions &options) {
    const auto getOutlineVisibility = [&](const Entity &entity)
        -> std::optional<OutlineVisibility> {
        const MeshRendererComponent &component = *entity.meshRenderComponent;
        const bool highlighted = options.highlightedEntityId &&
                                 isEntityOrDescendantOf(
                                     scene,
                                     entity,
                                     *options.highlightedEntityId
                                 );

        if (highlighted) {
            return OutlineVisibility::AlwaysVisible;
        }

        if (!component.outlineEnabled) {
            return std::nullopt;
        }

        return component.outlineVisibility;
    };

    bool hasVisibleOutline = false;
    bool hasAlwaysVisibleOutline = false;

    // Scene mesh pass: finish the depth buffer and mark visible-only outlines.
    for (const Entity &entity: scene.getEntities()) {
        if (entity.meshRenderComponent) {
            const MeshRendererComponent &component = *entity.meshRenderComponent;
            const std::optional<OutlineVisibility> visibility = getOutlineVisibility(entity);
            const bool visibleOnly = visibility == OutlineVisibility::VisibleOnly;

            auto worldMatrix = scene.getWorldMatrix(entity);
            meshRenderPass(worldMatrix, *component.mesh, *component.material, visibleOnly);

            hasVisibleOutline |= visibleOnly;
            hasAlwaysVisibleOutline |= visibility == OutlineVisibility::AlwaysVisible;
        }
    }

    if (hasVisibleOutline) {
        // Visible-only outline pass: use the completed scene depth buffer.
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        for (const Entity &entity: scene.getEntities()) {
            if (!entity.meshRenderComponent ||
                getOutlineVisibility(entity) != OutlineVisibility::VisibleOnly) {
                continue;
            }

            const MeshRendererComponent &component = *entity.meshRenderComponent;
            const glm::mat4 worldMatrix = scene.getWorldMatrix(entity);
            drawMeshOutline(worldMatrix, *component.mesh, component.outlineMode, outlineWidth);
        }

        glDepthMask(GL_TRUE);
        glStencilMask(0xFF);
        glDisable(GL_STENCIL_TEST);
    }

    if (hasAlwaysVisibleOutline) {
        // Rebuild stencil from the complete silhouettes of X-ray targets.
        glStencilMask(0xFF);
        glClear(GL_STENCIL_BUFFER_BIT);
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        for (const Entity &entity: scene.getEntities()) {
            if (!entity.meshRenderComponent ||
                getOutlineVisibility(entity) != OutlineVisibility::AlwaysVisible) {
                continue;
            }

            const MeshRendererComponent &component = *entity.meshRenderComponent;
            const glm::mat4 worldMatrix = scene.getWorldMatrix(entity);
            drawMeshOutline(worldMatrix, *component.mesh, component.outlineMode, 0.0f);
        }

        // Draw only outside the original X-ray silhouettes, ignoring scene depth.
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

        for (const Entity &entity: scene.getEntities()) {
            if (!entity.meshRenderComponent ||
                getOutlineVisibility(entity) != OutlineVisibility::AlwaysVisible) {
                continue;
            }

            const MeshRendererComponent &component = *entity.meshRenderComponent;
            const glm::mat4 worldMatrix = scene.getWorldMatrix(entity);
            drawMeshOutline(worldMatrix, *component.mesh, component.outlineMode, outlineWidth);
        }

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glStencilMask(0xFF);
        glDisable(GL_STENCIL_TEST);
    }
}

void Renderer::endFrame() {
}
