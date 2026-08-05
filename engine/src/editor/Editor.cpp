#include "Editor.h"
#include "EditorTheme.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <ImGuizmo.h>
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../core/Input.h"
#include "../rendering/mesh/InstancedMeshRendererComponent.h"
#include "../rendering/model/InstancedModelRendererComponent.h"
#include "../resources/ResourceManager.h"
#include "../scene/Scene.h"
#include "../scene/SceneRaycaster.h"

namespace {
constexpr const char *frameBufferDebugViewNames[] = {
    "Off",
    "G-buffer Position",
    "G-buffer Normal",
    "G-buffer Albedo",
    "G-buffer Specular",
    "G-buffer Depth",
    "SSAO Raw",
    "SSAO Blurred",
    "Directional Shadow",
    "Point Shadow"
};

const char *getFrameBufferDebugViewName(const FrameBufferDebugView view) {
    const int index = static_cast<int>(view);
    if (index < 0 || index >= IM_ARRAYSIZE(frameBufferDebugViewNames)) {
        return "Framebuffer";
    }
    return frameBufferDebugViewNames[index];
}
}

Editor::Editor(
    const ComponentTypeRegistry &componentTypes,
    ResourceManager &resources
)
    : componentTypes(componentTypes),
      resources(resources) {
    EditorTheme::applyModernDark();
    registerDefaultComponentDrawers(
        componentDrawers,
        resources,
        selectedInstance
    );
}

std::optional<PlayModeRequest> Editor::consumePlayModeRequest() {
    const std::optional<PlayModeRequest> result = pendingPlayModeRequest;
    pendingPlayModeRequest.reset();
    return result;
}

void Editor::beginFrame() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
    const bool needsDefaultLayout = ImGui::DockBuilderGetNode(dockspaceId) == nullptr;

    ImGui::DockSpaceOverViewport(
        dockspaceId,
        viewport,
        ImGuiDockNodeFlags_None
    );

    if (!needsDefaultLayout) {
        return;
    }

    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(
        dockspaceId,
        ImGuiDockNodeFlags_DockSpace
    );
    ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

    ImGuiID centerId = dockspaceId;
    const ImGuiID leftId = ImGui::DockBuilderSplitNode(
        centerId,
        ImGuiDir_Left,
        0.20f,
        nullptr,
        &centerId
    );
    ImGuiID rightId = ImGui::DockBuilderSplitNode(
        centerId,
        ImGuiDir_Right,
        0.25f,
        nullptr,
        &centerId
    );
    const ImGuiID debugId = ImGui::DockBuilderSplitNode(
        rightId,
        ImGuiDir_Down,
        0.35f,
        nullptr,
        &rightId
    );

    ImGui::DockBuilderDockWindow("Hierarchy", leftId);
    ImGui::DockBuilderDockWindow("Scene", centerId);
    ImGui::DockBuilderDockWindow("Inspector", rightId);
    ImGui::DockBuilderDockWindow("Debug", debugId);
    ImGui::DockBuilderFinish(dockspaceId);
}

void Editor::draw(
    Scene &scene,
    const WindowSize& windowSize,
    const MouseState& mouseState,
    RenderSettings& renderSettings,
    const RenderStats& renderStats
) {
    drawHierarchy(scene);
    drawInspector(scene);
    drawDebug(scene, windowSize, mouseState, renderSettings, renderStats);
}

void Editor::drawInspector(Scene &scene) {
    const bool visible = ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoCollapse);
    if (visible) {
        ImGui::PushTextWrapPos(0.0f);
        drawInspectorContent(scene);
        ImGui::PopTextWrapPos();
    }
    ImGui::End();
}

void Editor::drawInspectorContent(Scene &scene) {
    if (!selectedEntityId) {
        ImGui::TextDisabled("No entity selected");
        return;
    }

    Entity *entity = scene.findEntity(*selectedEntityId);
    if (!entity) {
        selectedEntityId.reset();
        return;
    }

    if (ImGui::BeginTable("##EntityHeader", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Name");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputText("##EntityName", &entity->name);
        if (ImGui::IsItemDeactivatedAfterEdit() && entity->name.empty()) {
            entity->name = "Entity";
        }
        ImGui::EndTable();
    }

    ImGui::TextDisabled(
        "ID: %llu",
        static_cast<unsigned long long>(entity->id)
    );
    componentDrawers.drawComponents(scene, *entity, componentTypes);
    componentDrawers.drawAddComponent(*entity, componentTypes);
}

void Editor::drawDebug(
    Scene &scene,
    const WindowSize& windowSize,
    const MouseState& mouseState,
    RenderSettings& renderSettings,
    const RenderStats& renderStats
) {
    if (ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::SeparatorText("Performance");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text(
            "Total Draw Calls: %llu",
            static_cast<unsigned long long>(renderStats.drawCalls)
        );
        ImGui::Text(
            "Shadow Draw Calls: %llu",
            static_cast<unsigned long long>(renderStats.shadowDrawCalls)
        );
        ImGui::Text(
            "Instanced Draw Calls: %llu",
            static_cast<unsigned long long>(renderStats.instancedDrawCalls)
        );
        ImGui::Text(
            "Submitted Mesh Instances: %llu",
            static_cast<unsigned long long>(renderStats.instanceCount)
        );
        ImGui::Text(
            "Submitted Triangles: %llu",
            static_cast<unsigned long long>(renderStats.triangleCount)
        );
        ImGui::Text(
            "Shadow Triangles: %llu",
            static_cast<unsigned long long>(renderStats.shadowTriangleCount)
        );
        if (renderSettings.renderingPath == RenderingPath::Deferred) {
            ImGui::Text(
                "Deferred Geometry Calls: %llu",
                static_cast<unsigned long long>(
                    renderStats.deferredGeometryDrawCalls
                )
            );
            ImGui::Text(
                "Deferred Lighting Calls: %llu",
                static_cast<unsigned long long>(
                    renderStats.deferredLightingDrawCalls
                )
            );
            ImGui::Text(
                "SSAO Calls: %llu",
                static_cast<unsigned long long>(renderStats.ssaoDrawCalls)
            );
        }
        ImGui::Text(
            "Framebuffer Debug Calls: %llu",
            static_cast<unsigned long long>(
                renderStats.frameBufferDebugDrawCalls
            )
        );
        const std::uint64_t estimatedSavedCalls =
                renderStats.instanceCount > renderStats.instancedDrawCalls
                    ? renderStats.instanceCount - renderStats.instancedDrawCalls
                    : 0;
        ImGui::Text(
            "Estimated Calls Saved: %llu",
            static_cast<unsigned long long>(estimatedSavedCalls)
        );
        ImGui::TextDisabled("Scene renderer only; post-process and ImGui excluded.");

        ImGui::SeparatorText("Rendering");
        constexpr const char *renderingPathNames[] = {
            "Forward",
            "Deferred"
        };
        int renderingPath = static_cast<int>(renderSettings.renderingPath);
        if (ImGui::Combo(
            "Rendering Path",
            &renderingPath,
            renderingPathNames,
            IM_ARRAYSIZE(renderingPathNames)
        )) {
            renderSettings.renderingPath =
                    static_cast<RenderingPath>(renderingPath);
            if (renderSettings.renderingPath == RenderingPath::Forward &&
                renderSettings.debugView >= DebugViewMode::GBufferPosition) {
                renderSettings.debugView = DebugViewMode::Shaded;
            }
        }

        constexpr const char *debugViewNames[] = {
            "Shaded",
            "Depth",
            "World Normal",
            "G-buffer Position",
            "G-buffer Albedo",
            "G-buffer Specular",
            "SSAO"
        };
        int debugView = static_cast<int>(renderSettings.debugView);
        const int debugViewCount =
                renderSettings.renderingPath == RenderingPath::Deferred
                    ? IM_ARRAYSIZE(debugViewNames)
                    : 3;
        if (ImGui::Combo(
            "View Mode",
            &debugView,
            debugViewNames,
            debugViewCount
        )) {
            renderSettings.debugView =
                    static_cast<DebugViewMode>(debugView);
        }

        ImGui::SeparatorText("Framebuffer Preview");
        int frameBufferDebugView =
                static_cast<int>(renderSettings.frameBufferDebugView);
        if (ImGui::BeginCombo(
            "Attachment",
            getFrameBufferDebugViewName(
                renderSettings.frameBufferDebugView
            )
        )) {
            for (int index = 0;
                 index < IM_ARRAYSIZE(frameBufferDebugViewNames);
                 ++index) {
                const auto candidate =
                        static_cast<FrameBufferDebugView>(index);
                const bool needsDeferred =
                        candidate >= FrameBufferDebugView::GBufferPosition &&
                        candidate <= FrameBufferDebugView::SSAOBlurred;
                bool supported =
                        !needsDeferred ||
                        renderSettings.renderingPath == RenderingPath::Deferred;
                if (candidate == FrameBufferDebugView::SSAORaw ||
                    candidate == FrameBufferDebugView::SSAOBlurred) {
                    supported = supported && renderSettings.ssaoEnabled;
                } else if (candidate ==
                           FrameBufferDebugView::DirectionalShadow) {
                    supported = renderSettings.shadowsEnabled;
                } else if (candidate ==
                           FrameBufferDebugView::PointShadow) {
                    supported = renderSettings.pointShadowsEnabled;
                }
                ImGui::BeginDisabled(!supported);
                if (ImGui::Selectable(
                    frameBufferDebugViewNames[index],
                    frameBufferDebugView == index
                )) {
                    frameBufferDebugView = index;
                    renderSettings.frameBufferDebugView = candidate;
                }
                ImGui::EndDisabled();
            }
            ImGui::EndCombo();
        }
        if (renderSettings.renderingPath == RenderingPath::Forward &&
            renderSettings.frameBufferDebugView >=
                FrameBufferDebugView::GBufferPosition &&
            renderSettings.frameBufferDebugView <=
                FrameBufferDebugView::SSAOBlurred) {
            renderSettings.frameBufferDebugView =
                    FrameBufferDebugView::Off;
        }
        if (renderSettings.frameBufferDebugView !=
            FrameBufferDebugView::Off) {
            ImGui::SetNextItemWidth(120.0f);
            ImGui::SliderFloat(
                "Size",
                &renderSettings.frameBufferDebugScale,
                0.15f,
                0.5f,
                "%.2f"
            );
        }
        if (renderSettings.frameBufferDebugView ==
            FrameBufferDebugView::PointShadow) {
            constexpr const char *cubeFaceNames[] = {
                "+X", "-X", "+Y", "-Y", "+Z", "-Z"
            };
            ImGui::Combo(
                "Cubemap Face",
                &renderSettings.pointShadowDebugFace,
                cubeFaceNames,
                IM_ARRAYSIZE(cubeFaceNames)
            );
        }
        ImGui::TextDisabled(
            "Displays the selected attachment over the Scene view."
        );

        bool wireframe = renderSettings.rasterization ==
                         RasterizationMode::Wireframe;
        if (ImGui::Checkbox("Wireframe", &wireframe)) {
            renderSettings.rasterization = wireframe
                                               ? RasterizationMode::Wireframe
                                               : RasterizationMode::Fill;
        }

        constexpr int msaaSampleCounts[] = {1, 2, 4, 8};
        constexpr const char *msaaLabels[] = {"Off", "2x", "4x", "8x"};
        int currentMsaaIndex = 0;
        for (int index = 0; index < IM_ARRAYSIZE(msaaSampleCounts); ++index) {
            if (renderSettings.msaaSamples == msaaSampleCounts[index]) {
                currentMsaaIndex = index;
                break;
            }
        }

        GLint maximumSamples = 1;
        glGetIntegerv(GL_MAX_SAMPLES, &maximumSamples);
        const bool deferredRendering =
                renderSettings.renderingPath == RenderingPath::Deferred;
        ImGui::BeginDisabled(deferredRendering);
        if (ImGui::BeginCombo("MSAA", msaaLabels[currentMsaaIndex])) {
            for (int index = 0; index < IM_ARRAYSIZE(msaaSampleCounts); ++index) {
                const bool supported = msaaSampleCounts[index] <= maximumSamples;
                ImGui::BeginDisabled(!supported);
                if (ImGui::Selectable(
                    msaaLabels[index],
                    currentMsaaIndex == index
                )) {
                    renderSettings.msaaSamples = msaaSampleCounts[index];
                }
                ImGui::EndDisabled();
            }
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();
        ImGui::TextDisabled("Maximum supported samples: %d", maximumSamples);
        if (deferredRendering) {
            ImGui::TextDisabled(
                "Deferred rendering currently uses single-sample G-buffers."
            );
        }

        ImGui::SeparatorText("Screen-Space Ambient Occlusion");
        ImGui::BeginDisabled(!deferredRendering);
        ImGui::Checkbox("Enabled##SSAO", &renderSettings.ssaoEnabled);
        if (renderSettings.ssaoEnabled) {
            ImGui::SetNextItemWidth(120.0f);
            ImGui::SliderInt(
                "Samples##SSAO",
                &renderSettings.ssaoSampleCount,
                8,
                64
            );
            ImGui::SetNextItemWidth(120.0f);
            ImGui::DragFloat(
                "Radius##SSAO",
                &renderSettings.ssaoRadius,
                0.01f,
                0.05f,
                5.0f,
                "%.2f"
            );
            ImGui::SetNextItemWidth(120.0f);
            ImGui::DragFloat(
                "Bias##SSAO",
                &renderSettings.ssaoBias,
                0.001f,
                0.0f,
                0.25f,
                "%.3f"
            );
            ImGui::SetNextItemWidth(120.0f);
            ImGui::DragFloat(
                "Power##SSAO",
                &renderSettings.ssaoPower,
                0.05f,
                0.1f,
                8.0f,
                "%.2f"
            );
        }
        ImGui::EndDisabled();
        ImGui::TextDisabled(
            "SSAO affects the deferred ambient-light term only."
        );

        ImGui::SeparatorText("Color Pipeline");
        ImGui::Checkbox("HDR", &renderSettings.hdrEnabled);
        if (renderSettings.hdrEnabled) {
            constexpr const char *toneMappingNames[] = {
                "Reinhard",
                "Exposure"
            };
            int toneMapping = static_cast<int>(renderSettings.toneMapping);
            if (ImGui::Combo(
                "Tone Mapping",
                &toneMapping,
                toneMappingNames,
                IM_ARRAYSIZE(toneMappingNames)
            )) {
                renderSettings.toneMapping =
                        static_cast<ToneMappingMode>(toneMapping);
            }

            if (renderSettings.toneMapping == ToneMappingMode::Exposure) {
                ImGui::SetNextItemWidth(120.0f);
                ImGui::DragFloat(
                    "Exposure",
                    &renderSettings.exposure,
                    0.01f,
                    0.0f,
                    10.0f,
                    "%.2f"
                );
            }
        }
        ImGui::TextDisabled(
            "Scene: RGBA16F, final editor image: RGBA8."
        );
        ImGui::TextDisabled(
            "Tone mapping is bypassed for debug views."
        );

        ImGui::Checkbox("Bloom", &renderSettings.bloomEnabled);
        if (renderSettings.bloomEnabled) {
            ImGui::SetNextItemWidth(120.0f);
            ImGui::DragFloat(
                "Threshold",
                &renderSettings.bloomThreshold,
                0.01f,
                0.0f,
                20.0f,
                "%.2f"
            );
            ImGui::SetNextItemWidth(120.0f);
            ImGui::DragFloat(
                "Strength",
                &renderSettings.bloomStrength,
                0.01f,
                0.0f,
                2.0f,
                "%.2f"
            );
            ImGui::SetNextItemWidth(120.0f);
            ImGui::SliderInt(
                "Blur Passes",
                &renderSettings.bloomBlurPasses,
                0,
                32
            );
        }
        ImGui::TextDisabled(
            "Bloom is combined before tone mapping."
        );

        ImGui::Checkbox(
            "Gamma Correction",
            &renderSettings.gammaCorrection
        );
        if (renderSettings.gammaCorrection) {
            ImGui::SetNextItemWidth(120.0f);
            ImGui::DragFloat(
                "Gamma",
                &renderSettings.gamma,
                0.01f,
                1.0f,
                4.0f,
                "%.2f"
            );
        }
        ImGui::TextDisabled(
            "Applied once to the final scene image."
        );

        ImGui::SeparatorText("Directional Shadows");
        ImGui::Checkbox(
            "Enabled##DirectionalShadows",
            &renderSettings.shadowsEnabled
        );
        if (renderSettings.shadowsEnabled) {
            constexpr int shadowResolutions[] = {512, 1024, 2048, 4096};
            constexpr const char *shadowResolutionLabels[] = {
                "512",
                "1024",
                "2048",
                "4096"
            };
            int resolutionIndex = 0;
            for (int index = 0;
                 index < IM_ARRAYSIZE(shadowResolutions);
                 ++index) {
                if (renderSettings.shadowMapResolution ==
                    shadowResolutions[index]) {
                    resolutionIndex = index;
                    break;
                }
            }

            GLint maximumTextureSize = 1;
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTextureSize);
            if (ImGui::BeginCombo(
                "Resolution",
                shadowResolutionLabels[resolutionIndex]
            )) {
                for (int index = 0;
                     index < IM_ARRAYSIZE(shadowResolutions);
                     ++index) {
                    const bool supported =
                            shadowResolutions[index] <= maximumTextureSize;
                    ImGui::BeginDisabled(!supported);
                    if (ImGui::Selectable(
                        shadowResolutionLabels[index],
                        resolutionIndex == index
                    )) {
                        renderSettings.shadowMapResolution =
                                shadowResolutions[index];
                    }
                    ImGui::EndDisabled();
                }
                ImGui::EndCombo();
            }
            ImGui::DragFloat(
                "Distance",
                &renderSettings.shadowDistance,
                0.25f,
                1.0f,
                200.0f,
                "%.1f"
            );
            ImGui::DragFloat(
                "Minimum Bias",
                &renderSettings.shadowBiasMin,
                0.00005f,
                0.0f,
                0.05f,
                "%.5f"
            );
            ImGui::DragFloat(
                "Slope Bias",
                &renderSettings.shadowBiasSlope,
                0.0001f,
                0.0f,
                0.1f,
                "%.4f"
            );
            ImGui::SliderInt(
                "PCF Radius",
                &renderSettings.shadowPcfRadius,
                0,
                3
            );
            ImGui::TextDisabled(
                "The first enabled directional light with Cast Shadows is used."
            );
        }

        ImGui::SeparatorText("Point Shadows");
        ImGui::Checkbox(
            "Enabled##PointShadows",
            &renderSettings.pointShadowsEnabled
        );
        if (renderSettings.pointShadowsEnabled) {
            constexpr int pointShadowResolutions[] = {256, 512, 1024, 2048};
            constexpr const char *pointShadowResolutionLabels[] = {
                "256",
                "512",
                "1024",
                "2048"
            };
            int resolutionIndex = 0;
            for (int index = 0;
                 index < IM_ARRAYSIZE(pointShadowResolutions);
                 ++index) {
                if (renderSettings.pointShadowMapResolution ==
                    pointShadowResolutions[index]) {
                    resolutionIndex = index;
                    break;
                }
            }

            GLint maximumTextureSize = 1;
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTextureSize);
            if (ImGui::BeginCombo(
                "Resolution##PointShadows",
                pointShadowResolutionLabels[resolutionIndex]
            )) {
                for (int index = 0;
                     index < IM_ARRAYSIZE(pointShadowResolutions);
                     ++index) {
                    const bool supported =
                            pointShadowResolutions[index] <= maximumTextureSize;
                    ImGui::BeginDisabled(!supported);
                    if (ImGui::Selectable(
                        pointShadowResolutionLabels[index],
                        resolutionIndex == index
                    )) {
                        renderSettings.pointShadowMapResolution =
                                pointShadowResolutions[index];
                    }
                    ImGui::EndDisabled();
                }
                ImGui::EndCombo();
            }
            ImGui::DragFloat(
                "Bias##PointShadows",
                &renderSettings.pointShadowBias,
                0.001f,
                0.0f,
                1.0f,
                "%.3f"
            );
            ImGui::DragFloat(
                "Softness##PointShadows",
                &renderSettings.pointShadowSoftness,
                0.001f,
                0.0f,
                0.5f,
                "%.3f"
            );
            ImGui::SliderInt(
                "PCF Samples##PointShadows",
                &renderSettings.pointShadowSampleCount,
                1,
                20
            );
            ImGui::TextDisabled(
                "The first enabled point light with Cast Shadows is used."
            );
            ImGui::TextDisabled(
                "The light Range is also the shadow far plane."
            );
        }

        if (renderSettings.debugView == DebugViewMode::Depth ||
            renderSettings.frameBufferDebugView ==
                FrameBufferDebugView::GBufferDepth) {
            ImGui::DragFloatRange2(
                "Depth Range",
                &renderSettings.debugDepthNear,
                &renderSettings.debugDepthFar,
                0.1f,
                0.0f,
                1000.0f,
                "Near: %.1f",
                "Far: %.1f"
            );
        }

        ImGui::SeparatorText("Window");
        ImGui::Text("Logical: %d x %d", windowSize.w, windowSize.h);
        ImGui::Text("Framebuffer: %d x %d", windowSize.fb_w, windowSize.fb_h);

        ImGui::SeparatorText("Input");
        ImGui::Text("Cursor: %.2f, %.2f", mouseState.screenX, mouseState.screenY);
        ImGui::Text("Delta: %.2f, %.2f", mouseState.deltaX, mouseState.deltaY);
        if (const std::optional<glm::vec2> scenePosition =
                sceneViewport.windowToLocal({mouseState.screenX, mouseState.screenY})) {
            ImGui::Text("Scene Cursor: %.2f, %.2f", scenePosition->x, scenePosition->y);
        } else {
            ImGui::TextDisabled("Scene Cursor: Outside");
        }
        ImGui::Text("Left: %s", mouseState.leftBtnDown ? "Pressed" : "None");
        ImGui::Text("Right: %s", mouseState.rightBtnDown ? "Pressed" : "None");

        ImGui::SeparatorText("Camera");
        const std::optional<EntityId> activeCameraId = scene.getActiveCameraId();
        const Entity *configuredCameraEntity = activeCameraId
                                                   ? scene.findEntity(*activeCameraId)
                                                   : nullptr;
        const char *activeCameraPreview = configuredCameraEntity
                                              ? configuredCameraEntity->name.c_str()
                                              : "None";

        if (ImGui::BeginCombo("Active Camera", activeCameraPreview)) {
            scene.each<CameraComponent>(
                [&](Entity &entity, CameraComponent &) {
                    const bool selected = activeCameraId && *activeCameraId == entity.id;
                    const std::string label = std::format("{}##Camera_{}", entity.name, entity.id);
                    if (ImGui::Selectable(label.c_str(), selected)) {
                        scene.setActiveCamera(entity.id);
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
            );
            ImGui::EndCombo();
        }

        if (Entity *cameraEntity = scene.getActiveCameraEntity()) {
            Transform &transform = cameraEntity->getComponent<TransformComponent>().local;
            CameraComponent &camera = cameraEntity->getComponent<CameraComponent>();

            ImGui::Text("Active: %s", cameraEntity->name.c_str());
            ImGui::DragFloat3("Position##Camera", glm::value_ptr(transform.position), 1.0f);
            ImGui::DragFloat3("Rotation##Camera", glm::value_ptr(transform.rotation), 1.0f);

            const bool isPerspective = std::holds_alternative<PerspectiveProjection>(camera.projection);
            if (ImGui::BeginCombo("Projection", isPerspective ? "Perspective" : "Orthographic")) {
                if (ImGui::Selectable("Perspective", isPerspective)) {
                    camera.projection = PerspectiveProjection{};
                }
                if (ImGui::Selectable("Orthographic", !isPerspective)) {
                    camera.projection = OrthoGraphicProjection{};
                }
                ImGui::EndCombo();
            }
        } else {
            ImGui::TextDisabled("No active camera");
        }
    }
    ImGui::End();
}

void Editor::drawHierarchy(Scene &scene) {
    if (ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse)) {
        drawSiblingList(scene, std::nullopt);

        if (ImGui::BeginPopupContextWindow(
            "HierarchyContext",
            ImGuiPopupFlags_MouseButtonRight |
            ImGuiPopupFlags_NoOpenOverItems
        )) {
            drawEntityCreationMenu(std::nullopt);
            ImGui::EndPopup();
        }

        const bool deletePressed =
                ImGui::IsKeyPressed(ImGuiKey_Backspace) ||
                ImGui::IsKeyPressed(ImGuiKey_Delete);
        if (selectedEntityId &&
            deletePressed &&
            !ImGui::GetIO().WantTextInput &&
            !ImGui::IsAnyItemActive()) {
            pendingEntityDeletion = selectedEntityId;
        }
    }
    ImGui::End();

    if (pendingEntityDeletion) {
        scene.destroyEntity(*pendingEntityDeletion);
        pendingEntityDeletion.reset();

        if (selectedEntityId && !scene.findEntity(*selectedEntityId)) {
            selectedEntityId.reset();
        }
    }

    if (pendingMoveReq) {
        scene.moveEntity(pendingMoveReq->sourceId, pendingMoveReq->newParentId, pendingMoveReq->insertIndex);
        pendingMoveReq.reset();
    }

    if (pendingEntityCreation) {
        const std::optional<EntityId> parentId = pendingEntityCreation->parentId;
        Entity &created = scene.createEntity(pendingEntityCreation->name);

        const std::span<const ResourceEntry<Material>> materials =
                resources.getMaterialResources();
        const Material *defaultMaterial = materials.empty()
                                              ? nullptr
                                              : materials.front().resource;

        if (pendingEntityCreation->type == EntityCreationType::InstancedMesh) {
            InstancedMeshRendererComponent &component =
                    created.addComponent<InstancedMeshRendererComponent>(
                        pendingEntityCreation->mesh,
                        defaultMaterial
                    );
            (void)component.addInstance();
        } else if (pendingEntityCreation->type == EntityCreationType::InstancedModel) {
            InstancedModelRendererComponent &component =
                    created.addComponent<InstancedModelRendererComponent>(
                        pendingEntityCreation->model,
                        defaultMaterial
                    );
            (void)component.addInstance();
        }

        if (parentId) {
            scene.moveEntity(
                created.id,
                parentId,
                scene.getChildren(parentId).size(),
                true
            );
        }

        selectedEntityId = created.id;
        pendingEntityCreation.reset();
    }
}

void Editor::drawEntityCreationMenu(std::optional<EntityId> parentId) {
    if (ImGui::MenuItem("Create Empty Entity")) {
        pendingEntityCreation = EntityCreationRequest{
            .parentId = parentId
        };
    }

    if (ImGui::BeginMenu("Create Instanced Mesh")) {
        const std::span<const ResourceEntry<Mesh>> meshes =
                resources.getMeshResources();
        if (meshes.empty()) {
            ImGui::TextDisabled("No meshes loaded");
        }
        for (const ResourceEntry<Mesh> &entry : meshes) {
            if (ImGui::MenuItem(entry.name.c_str())) {
                pendingEntityCreation = EntityCreationRequest{
                    .parentId = parentId,
                    .type = EntityCreationType::InstancedMesh,
                    .mesh = entry.resource,
                    .name = "Instanced " + entry.name
                };
            }
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Create Instanced Model")) {
        const std::span<const ResourceEntry<Model>> models =
                resources.getModelResources();
        if (models.empty()) {
            ImGui::TextDisabled("No models loaded");
        }
        for (const ResourceEntry<Model> &entry : models) {
            if (ImGui::MenuItem(entry.name.c_str())) {
                pendingEntityCreation = EntityCreationRequest{
                    .parentId = parentId,
                    .type = EntityCreationType::InstancedModel,
                    .model = entry.resource,
                    .name = "Instanced " + entry.name
                };
            }
        }
        ImGui::EndMenu();
    }
}

void Editor::drawSiblingList(Scene &scene, std::optional<EntityId> id) {
    auto siblings = scene.getChildren(id);

    drawInsertionSlot(id, 0);

    for (size_t i = 0; i < siblings.size(); i++) {
        drawEntityNode(scene, *siblings[i]);
        drawInsertionSlot(id, i + 1);
    }
}

void Editor::drawInsertionSlot(std::optional<EntityId> id, size_t insertIndex) {
    auto slotId = std::format("##DropSlot_{}_{}", id.value_or(std::numeric_limits<EntityId>::max()), insertIndex);
    const float availableWidth = ImGui::GetContentRegionAvail().x;

    if (availableWidth <= 0.0f) {
        return;
    }

    ImGui::InvisibleButton(slotId.c_str(), ImVec2(availableWidth, 2.0f));

    if (!ImGui::BeginDragDropTarget()) {
        return;
    }

    constexpr ImGuiDragDropFlags flags = ImGuiDragDropFlags_AcceptBeforeDelivery |
                                         ImGuiDragDropFlags_AcceptNoDrawDefaultRect;
    const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY", flags);

    if (payload) {
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        float y = (min.y + max.y) / 2.0f;
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(min.x, y),
            ImVec2(max.x, y),
            ImGui::GetColorU32(ImGuiCol_DragDropTarget),
            2.0f
        );

        if (payload->IsDelivery()) {
            const EntityId sourceId = *static_cast<const EntityId *>(payload->Data);
            pendingMoveReq = {
                .sourceId = sourceId,
                .newParentId = id,
                .insertIndex = insertIndex
            };
        }
    }

    ImGui::EndDragDropTarget();
}

RenderExtent Editor::drawSceneView(
    Scene &scene,
    GLuint textureId,
    GLuint frameBufferDebugTextureId,
    const RenderSettings &renderSettings,
    bool isPlaying,
    const MouseState &mouseState
) {
    constexpr ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const bool visible = ImGui::Begin("Scene", nullptr, windowFlags);
    ImGui::PopStyleVar();

    if (!visible) {
        ImGui::End();
        return {};
    }

    const auto drawGizmoButton = [&](const char *label, const char *tooltip,
                                     GizmoOperation operation) {
        const bool selected = gizmoOperation == operation;
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }
        if (ImGui::Button(label, ImVec2(28.0f, 0.0f))) {
            gizmoOperation = operation;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }
        if (selected) {
            ImGui::PopStyleColor();
        }
    };

    drawGizmoButton("Q", "Translate (Q)", GizmoOperation::Translate);
    ImGui::SameLine();
    drawGizmoButton("W", "Rotate (W)", GizmoOperation::Rotate);
    ImGui::SameLine();
    drawGizmoButton("E", "Scale (E)", GizmoOperation::Scale);

    const ImGuiIO &io = ImGui::GetIO();
    const bool acceptsGizmoShortcut =
            ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            !io.WantTextInput &&
            !io.KeyCtrl &&
            !io.KeyAlt &&
            !io.KeySuper;
    if (acceptsGizmoShortcut) {
        if (ImGui::IsKeyPressed(ImGuiKey_Q, false)) {
            gizmoOperation = GizmoOperation::Translate;
        } else if (ImGui::IsKeyPressed(ImGuiKey_W, false)) {
            gizmoOperation = GizmoOperation::Rotate;
        } else if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
            gizmoOperation = GizmoOperation::Scale;
        }
    }

    constexpr float playButtonWidth = 72.0f;
    const float toolbarWidth =
            ImGui::GetWindowContentRegionMax().x -
            ImGui::GetWindowContentRegionMin().x;
    ImGui::SameLine();
    ImGui::SetCursorPosX(
        std::max(
            ImGui::GetCursorPosX(),
            ImGui::GetWindowContentRegionMin().x +
            (toolbarWidth - playButtonWidth) * 0.5f
        )
    );

    if (ImGui::Button(
        isPlaying ? "Stop" : "Play",
        ImVec2(playButtonWidth, 0.0f)
    )) {
        pendingPlayModeRequest = isPlaying
                                     ? PlayModeRequest::Stop
                                     : PlayModeRequest::Play;
    }
    ImGui::Separator();

    const ImVec2 available = ImGui::GetContentRegionAvail();
    const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;

    const RenderExtent extent{
        .width = static_cast<int>(available.x * scale.x),
        .height = static_cast<int>(available.y * scale.y)
    };

    if (extent.width > 0 && extent.height > 0) {
        ImGui::Image(textureId, available, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
        const ImVec2 imageMin = ImGui::GetItemRectMin();
        const ImVec2 imageSize = ImGui::GetItemRectSize();
        const ImVec2 mainViewportPosition = ImGui::GetMainViewport()->Pos;
        sceneViewport.set(
            {
                (imageMin.x - mainViewportPosition.x) * scale.x,
                (imageMin.y - mainViewportPosition.y) * scale.y
            },
            extent
        );

        const bool imageHovered = ImGui::IsItemHovered();
        drawSelectionGizmo(
            scene,
            imageMin,
            imageSize,
            extent
        );

        if (frameBufferDebugTextureId != 0 &&
            renderSettings.frameBufferDebugView !=
                FrameBufferDebugView::Off) {
            const float maximumWidth = std::max(imageSize.x - 24.0f, 1.0f);
            float previewWidth = std::min(
                std::max(
                    imageSize.x * renderSettings.frameBufferDebugScale,
                    140.0f
                ),
                maximumWidth
            );
            float previewHeight = previewWidth *
                                  imageSize.y /
                                  std::max(imageSize.x, 1.0f);
            const float maximumHeight = imageSize.y * 0.5f;
            if (previewHeight > maximumHeight) {
                previewHeight = maximumHeight;
                previewWidth = previewHeight *
                               imageSize.x /
                               std::max(imageSize.y, 1.0f);
            }

            const ImVec2 previewMin{
                imageMin.x + imageSize.x - previewWidth - 12.0f,
                imageMin.y + 12.0f
            };
            const ImVec2 previewMax{
                previewMin.x + previewWidth,
                previewMin.y + previewHeight
            };
            ImDrawList *drawList = ImGui::GetWindowDrawList();
            drawList->AddImage(
                frameBufferDebugTextureId,
                previewMin,
                previewMax,
                ImVec2(0.0f, 1.0f),
                ImVec2(1.0f, 0.0f)
            );
            drawList->AddRect(
                previewMin,
                previewMax,
                IM_COL32(220, 225, 235, 220),
                2.0f,
                0,
                1.0f
            );

            const float labelHeight = ImGui::GetTextLineHeight() + 8.0f;
            drawList->AddRectFilled(
                previewMin,
                ImVec2(previewMax.x, previewMin.y + labelHeight),
                IM_COL32(10, 13, 18, 205),
                2.0f
            );
            drawList->AddText(
                ImVec2(previewMin.x + 6.0f, previewMin.y + 4.0f),
                IM_COL32(235, 238, 245, 255),
                getFrameBufferDebugViewName(
                    renderSettings.frameBufferDebugView
                )
            );
        }

        if (!isPlaying &&
            imageHovered &&
            mouseState.leftBtnPressed &&
            !ImGuizmo::IsOver() &&
            !ImGuizmo::IsUsing()) {
            pickScene(scene, mouseState);
        }
    }

    ImGui::End();
    return extent;
}

void Editor::pickScene(Scene &scene, const MouseState &mouseState) {
    const std::optional<glm::vec2> localPosition =
            sceneViewport.windowToLocal({mouseState.screenX, mouseState.screenY});
    Entity *cameraEntity = scene.getActiveCameraEntity();
    if (!localPosition || !cameraEntity) {
        return;
    }

    const glm::vec2 ndc = sceneViewport.localToNdc(*localPosition);
    const CameraComponent &camera =
            cameraEntity->getComponent<CameraComponent>();
    const glm::mat4 cameraWorld = scene.getWorldMatrix(*cameraEntity);

    Transform cameraTransform;
    glm::mat4 viewMatrix;
    if (Transform::decompose(cameraWorld, cameraTransform)) {
        cameraTransform.scale = glm::vec3(1.0f);
        viewMatrix = glm::inverse(cameraTransform.getLocalMatrix());
    } else {
        viewMatrix = glm::inverse(cameraWorld);
    }

    const glm::mat4 projectionMatrix =
            camera.getProjectionMatrix(sceneViewport.getExtent());
    const glm::mat4 inverseViewProjection =
            glm::inverse(projectionMatrix * viewMatrix);

    glm::vec4 nearPoint = inverseViewProjection *
                          glm::vec4(ndc.x, ndc.y, -1.0f, 1.0f);
    glm::vec4 farPoint = inverseViewProjection *
                         glm::vec4(ndc.x, ndc.y, 1.0f, 1.0f);
    if (std::abs(nearPoint.w) <= 1e-6f || std::abs(farPoint.w) <= 1e-6f) {
        return;
    }
    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;

    const glm::vec3 direction = glm::vec3(farPoint - nearPoint);
    if (glm::dot(direction, direction) <= 1e-8f) {
        return;
    }

    const std::optional<SceneRaycastHit> hit = SceneRaycaster::cast(
        scene,
        Ray{
            .origin = glm::vec3(nearPoint),
            .direction = glm::normalize(direction)
        }
    );

    if (!hit) {
        selectedEntityId.reset();
        selectedInstance.reset();
        return;
    }

    selectedEntityId = hit->entityId;
    selectedInstance.reset();
    if (!hit->instanceId) {
        return;
    }

    selectedInstance = InstanceSelection{
        .entityId = hit->entityId,
        .instanceId = *hit->instanceId,
        .rendererType = hit->target == SceneRaycastTarget::MeshInstance
                            ? InstanceRendererType::Mesh
                            : InstanceRendererType::Model
    };
}

void Editor::drawSelectionGizmo(
    Scene &scene,
    ImVec2 imageMin,
    ImVec2 imageSize,
    const RenderExtent &extent
) {
    if (!selectedEntityId) {
        return;
    }

    Entity *entity = scene.findEntity(*selectedEntityId);
    Entity *cameraEntity = scene.getActiveCameraEntity();
    if (!entity || !cameraEntity) {
        if (!entity) {
            selectedEntityId.reset();
            selectedInstance.reset();
        }
        return;
    }

    InstanceData *instance = nullptr;
    if (selectedInstance && selectedInstance->entityId != entity->id) {
        selectedInstance.reset();
    }

    if (selectedInstance &&
        selectedInstance->rendererType == InstanceRendererType::Mesh) {
        if (auto *component =
                entity->tryGetComponent<InstancedMeshRendererComponent>()) {
            instance = component->findInstance(selectedInstance->instanceId);
        }
    } else if (selectedInstance) {
        if (auto *component =
                entity->tryGetComponent<InstancedModelRendererComponent>()) {
            instance = component->findInstance(selectedInstance->instanceId);
        }
    }

    if (selectedInstance && !instance) {
        selectedInstance.reset();
    }

    const glm::mat4 cameraWorldMatrix = scene.getWorldMatrix(*cameraEntity);
    Transform cameraWorldTransform;
    glm::mat4 viewMatrix;
    if (Transform::decompose(cameraWorldMatrix, cameraWorldTransform)) {
        cameraWorldTransform.scale = glm::vec3(1.0f);
        viewMatrix = glm::inverse(cameraWorldTransform.getLocalMatrix());
    } else {
        viewMatrix = glm::inverse(cameraWorldMatrix);
    }

    const CameraComponent &camera =
            cameraEntity->getComponent<CameraComponent>();
    const glm::mat4 projectionMatrix = camera.getProjectionMatrix(extent);
    const glm::mat4 entityWorldMatrix = scene.getWorldMatrix(*entity);
    glm::mat4 targetWorldMatrix = instance
                                      ? entityWorldMatrix *
                                        instance->transform.getLocalMatrix()
                                      : entityWorldMatrix;

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(imageMin.x, imageMin.y, imageSize.x, imageSize.y);
    ImGuizmo::SetOrthographic(
        std::holds_alternative<OrthoGraphicProjection>(camera.projection)
    );

    ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
    if (gizmoOperation == GizmoOperation::Rotate) {
        operation = ImGuizmo::ROTATE;
    } else if (gizmoOperation == GizmoOperation::Scale) {
        operation = ImGuizmo::SCALE;
    }

    ImGuizmo::Manipulate(
        glm::value_ptr(viewMatrix),
        glm::value_ptr(projectionMatrix),
        operation,
        ImGuizmo::LOCAL,
        glm::value_ptr(targetWorldMatrix)
    );

    if (!ImGuizmo::IsUsing()) {
        return;
    }

    if (instance) {
        const glm::mat4 localMatrix =
                glm::inverse(entityWorldMatrix) * targetWorldMatrix;
        (void)Transform::decompose(localMatrix, instance->transform);
        return;
    }

    glm::mat4 parentWorldMatrix{1.0f};
    if (entity->parentId) {
        if (const Entity *parent = scene.findEntity(*entity->parentId)) {
            parentWorldMatrix = scene.getWorldMatrix(*parent);
        }
    }

    const glm::mat4 localMatrix =
            glm::inverse(parentWorldMatrix) * targetWorldMatrix;
    Transform &localTransform =
            entity->getComponent<TransformComponent>().local;
    (void)Transform::decompose(localMatrix, localTransform);
}

void Editor::drawEntityNode(Scene &scene, const Entity &entity) {
    const auto children = scene.getChildren(entity.id);
    bool hasChildren = !children.empty();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (selectedEntityId && *selectedEntityId == entity.id) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (!hasChildren) {
        flags |= ImGuiTreeNodeFlags_Leaf;
        flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    std::string nodeLabel = std::format("{}##{}", entity.name, entity.id);
    bool opened = ImGui::TreeNodeEx(nodeLabel.c_str(), flags);

    if (ImGui::IsItemClicked()) {
        selectedEntityId = entity.id;
        selectedInstance.reset();
    }

    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("SCENE_ENTITY", &entity.id, sizeof(EntityId));
        ImGui::Text("Move %s", entity.name.c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY");
        if (payload && payload->IsDelivery()) {
            const EntityId sourceId = *static_cast<const EntityId *>(payload->Data);
            auto children = scene.getChildren(entity.id);
            pendingMoveReq = EntityMoveRequest{
                .sourceId = sourceId,
                .newParentId = entity.id,
                .insertIndex = children.size()
            };
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextItem()) {
        drawEntityCreationMenu(entity.id);
        ImGui::Separator();
        if (ImGui::MenuItem("Delete")) {
            pendingEntityDeletion = entity.id;
        }
        ImGui::EndPopup();
    }

    if (hasChildren && opened) {
        drawSiblingList(scene, entity.id);
        ImGui::TreePop();
    }
}
