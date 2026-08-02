#include "ComponentDrawerRegistry.h"

#include <algorithm>
#include <cfloat>
#include <vector>

#include <imgui.h>

#include "../camera/CameraComponent.h"
#include "../rendering/Lights.h"
#include "../rendering/mesh/MeshRendererComponent.h"
#include "../rendering/skybox/SkyboxComponent.h"
#include "../scene/Scene.h"
#include "../scene/TransformComponent.h"

namespace {
    bool drawVec3Control(
        const char *label,
        glm::vec3 &value,
        float resetValue,
        float step = 0.1f
    ) {
        bool changed = false;
        ImGui::PushID(label);

        constexpr ImGuiTableFlags tableFlags =
                ImGuiTableFlags_SizingStretchSame |
                ImGuiTableFlags_NoSavedSettings |
                ImGuiTableFlags_PadOuterX;

        if (ImGui::BeginTable("##Vec3Control", 4, tableFlags)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Z", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);

            const auto drawAxis = [&changed, resetValue, step](
                int column,
                const char *axisLabel,
                float &axisValue,
                ImVec4 buttonColor,
                ImVec4 hoveredColor,
                ImVec4 activeColor
            ) {
                ImGui::PushID(axisLabel);
                ImGui::TableSetColumnIndex(column);
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);

                const float buttonSize = ImGui::GetFrameHeight();
                if (ImGui::Button(axisLabel, ImVec2(buttonSize + 3.0f, buttonSize))) {
                    axisValue = resetValue;
                    changed = true;
                }

                ImGui::PopStyleColor(3);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-FLT_MIN);
                changed |= ImGui::DragFloat(
                    "##Value",
                    &axisValue,
                    step,
                    0.0f,
                    0.0f,
                    "%.2f"
                );

                ImGui::PopStyleVar();
                ImGui::PopID();
            };

            drawAxis(
                1, "X", value.x,
                {0.80f, 0.10f, 0.15f, 1.0f},
                {0.90f, 0.20f, 0.25f, 1.0f},
                {0.70f, 0.05f, 0.10f, 1.0f}
            );
            drawAxis(
                2, "Y", value.y,
                {0.20f, 0.70f, 0.20f, 1.0f},
                {0.30f, 0.80f, 0.30f, 1.0f},
                {0.10f, 0.60f, 0.10f, 1.0f}
            );
            drawAxis(
                3, "Z", value.z,
                {0.10f, 0.25f, 0.80f, 1.0f},
                {0.20f, 0.35f, 0.90f, 1.0f},
                {0.05f, 0.15f, 0.70f, 1.0f}
            );

            ImGui::EndTable();
        }

        ImGui::PopID();
        return changed;
    }

    void collectMeshRendererComponents(
        Scene &scene,
        EntityId rootId,
        std::vector<MeshRendererComponent *> &result
    ) {
        Entity *entity = scene.findEntity(rootId);
        if (!entity) {
            return;
        }

        if (auto *component = entity->tryGetComponent<MeshRendererComponent>()) {
            result.push_back(component);
        }

        for (const Entity *child: scene.getChildren(rootId)) {
            collectMeshRendererComponents(scene, child->id, result);
        }
    }

    std::vector<MeshRendererComponent *> collectMeshRendererComponents(
        Scene &scene,
        EntityId rootId
    ) {
        std::vector<MeshRendererComponent *> result;
        collectMeshRendererComponents(scene, rootId, result);
        return result;
    }

    void drawMeshRenderers(Scene &scene, Entity &entity) {
        std::vector<MeshRendererComponent *> components =
                collectMeshRendererComponents(scene, entity.id);
        if (components.empty()) {
            return;
        }

        MeshRendererComponent &first = *components.front();
        const bool componentEnabledMixed = std::ranges::any_of(
            components,
            [&](const MeshRendererComponent *component) {
                return component->enabled != first.enabled;
            }
        );

        if (componentEnabledMixed) {
            ImGui::TextUnformatted("Renderer Enabled: Mixed");
            if (ImGui::Button("Enable All Renderers")) {
                for (MeshRendererComponent *component: components) {
                    component->enabled = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Disable All Renderers")) {
                for (MeshRendererComponent *component: components) {
                    component->enabled = false;
                }
            }
        } else {
            bool enabled = first.enabled;
            if (ImGui::Checkbox("Renderer Enabled", &enabled)) {
                for (MeshRendererComponent *component: components) {
                    component->enabled = enabled;
                }
            }
        }

        const bool outlineEnabledMixed = std::ranges::any_of(
            components,
            [&](const MeshRendererComponent *component) {
                return component->outlineEnabled != first.outlineEnabled;
            }
        );

        if (outlineEnabledMixed) {
            ImGui::TextUnformatted("Persistent Outline: Mixed");
            if (ImGui::Button("Enable All Outlines")) {
                for (MeshRendererComponent *component: components) {
                    component->outlineEnabled = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Disable All Outlines")) {
                for (MeshRendererComponent *component: components) {
                    component->outlineEnabled = false;
                }
            }
        } else {
            bool outlineEnabled = first.outlineEnabled;
            if (ImGui::Checkbox("Persistent Outline", &outlineEnabled)) {
                for (MeshRendererComponent *component: components) {
                    component->outlineEnabled = outlineEnabled;
                }
            }
        }

        ImGui::TextDisabled(
            "%zu renderer(s) in this subtree. Selection highlight is independent.",
            components.size()
        );

        constexpr const char *outlineModeNames[] = {
            "Normal Extrusion",
            "Scale From Pivot"
        };
        const bool modeMixed = std::ranges::any_of(
            components,
            [&](const MeshRendererComponent *component) {
                return component->outlineMode != first.outlineMode;
            }
        );
        const int outlineMode = static_cast<int>(first.outlineMode);

        if (ImGui::BeginCombo(
            "Outline Mode",
            modeMixed ? "Mixed" : outlineModeNames[outlineMode]
        )) {
            for (int i = 0; i < IM_ARRAYSIZE(outlineModeNames); ++i) {
                if (ImGui::Selectable(
                    outlineModeNames[i],
                    !modeMixed && outlineMode == i
                )) {
                    for (MeshRendererComponent *component: components) {
                        component->outlineMode = static_cast<OutlineMode>(i);
                    }
                }
            }
            ImGui::EndCombo();
        }

        constexpr const char *outlineVisibilityNames[] = {
            "Visible Only",
            "Always Visible"
        };
        const bool visibilityMixed = std::ranges::any_of(
            components,
            [&](const MeshRendererComponent *component) {
                return component->outlineVisibility != first.outlineVisibility;
            }
        );
        const int outlineVisibility = static_cast<int>(first.outlineVisibility);

        if (ImGui::BeginCombo(
            "Outline Visibility",
            visibilityMixed ? "Mixed" : outlineVisibilityNames[outlineVisibility]
        )) {
            for (int i = 0; i < IM_ARRAYSIZE(outlineVisibilityNames); ++i) {
                if (ImGui::Selectable(
                    outlineVisibilityNames[i],
                    !visibilityMixed && outlineVisibility == i
                )) {
                    for (MeshRendererComponent *component: components) {
                        component->outlineVisibility =
                                static_cast<OutlineVisibility>(i);
                    }
                }
            }
            ImGui::EndCombo();
        }
    }
}

void registerDefaultComponentDrawers(ComponentDrawerRegistry &registry) {
    registry.registerDrawer<TransformComponent>(
        "Local Transform",
        [](Scene &, Entity &, TransformComponent &component) {
            Transform &transform = component.local;
            drawVec3Control("position", transform.position, 0.0f);
            drawVec3Control("rotation", transform.rotation, 0.0f);
            drawVec3Control("scale", transform.scale, 1.0f);
        }
    );

    registry.registerCustomDrawer(
        "Mesh Renderer",
        [](Scene &scene, Entity &entity) {
            return !collectMeshRendererComponents(scene, entity.id).empty();
        },
        drawMeshRenderers
    );

    registry.registerDrawer<CameraComponent>(
        "Camera",
        [](Scene &, Entity &, CameraComponent &camera) {
            ImGui::Checkbox("Enabled", &camera.enabled);
            bool perspective = std::holds_alternative<PerspectiveProjection>(camera.projection);

            if (ImGui::BeginCombo("Projection", perspective ? "Perspective" : "Orthographic")) {
                if (ImGui::Selectable("Perspective", perspective)) {
                    camera.projection = PerspectiveProjection{};
                    perspective = true;
                }
                if (ImGui::Selectable("Orthographic", !perspective)) {
                    camera.projection = OrthoGraphicProjection{};
                    perspective = false;
                }
                ImGui::EndCombo();
            }

            if (auto *projection = std::get_if<PerspectiveProjection>(&camera.projection)) {
                ImGui::DragFloat("Field of View", &projection->fov, 0.1f, 1.0f, 179.0f);
                ImGui::DragFloat("Near", &projection->near, 0.01f, 0.001f, projection->far);
                ImGui::DragFloat("Far", &projection->far, 1.0f, projection->near, 100000.0f);
            } else if (auto *projection = std::get_if<OrthoGraphicProjection>(&camera.projection)) {
                ImGui::DragFloat("Height", &projection->height, 0.1f, 0.001f, 100000.0f);
                ImGui::DragFloat("Near", &projection->near, 0.01f, -100000.0f, projection->far);
                ImGui::DragFloat("Far", &projection->far, 1.0f, projection->near, 100000.0f);
            }
        }
    );

    registry.registerDrawer<AmbientLightComponent>(
        "Ambient Light",
        [](Scene &, Entity &, AmbientLightComponent &light) {
            ImGui::Checkbox("Enabled", &light.enabled);
            ImGui::ColorEdit3("Color", &light.color.x);
            ImGui::DragFloat("Intensity", &light.intensity, 0.01f, 0.0f, 100.0f);
        }
    );

    registry.registerDrawer<DirectionalLightComponent>(
        "Directional Light",
        [](Scene &, Entity &, DirectionalLightComponent &light) {
            ImGui::Checkbox("Enabled", &light.enabled);
            ImGui::ColorEdit3("Color", &light.color.x);
            ImGui::DragFloat("Intensity", &light.intensity, 0.01f, 0.0f, 100.0f);
        }
    );

    registry.registerDrawer<PointLightComponent>(
        "Point Light",
        [](Scene &, Entity &, PointLightComponent &light) {
            ImGui::Checkbox("Enabled", &light.enabled);
            ImGui::ColorEdit3("Color", &light.color.x);
            ImGui::DragFloat("Intensity", &light.intensity, 0.01f, 0.0f, 100.0f);
            ImGui::DragFloat("Range", &light.range, 0.1f, 0.0f, 100000.0f);
        }
    );

    registry.registerDrawer<SpotLightComponent>(
        "Spot Light",
        [](Scene &, Entity &, SpotLightComponent &light) {
            ImGui::Checkbox("Enabled", &light.enabled);
            ImGui::ColorEdit3("Color", &light.color.x);
            ImGui::DragFloat("Intensity", &light.intensity, 0.01f, 0.0f, 100.0f);
            ImGui::DragFloat("Range", &light.range, 0.1f, 0.0f, 100000.0f);
            ImGui::DragFloat("Inner Angle", &light.innerAngle, 0.1f, 0.0f, 89.0f);
            ImGui::DragFloat("Outer Angle", &light.outerAngle, 0.1f, 0.0f, 89.0f);
            light.innerAngle = std::min(light.innerAngle, light.outerAngle);
        }
    );

    registry.registerDrawer<SkyboxComponent>(
        "Skybox",
        [](Scene &, Entity &, SkyboxComponent &skybox) {
            ImGui::Checkbox("Enabled", &skybox.enabled);
            ImGui::TextDisabled(
                "The skybox follows the camera; entity transform is ignored."
            );
        }
    );

    registry.registerComponent<MeshRendererComponent>("Mesh Renderer");
    registry.registerComponent<CameraComponent>("Camera");
    registry.registerComponent<AmbientLightComponent>("Ambient Light");
    registry.registerComponent<DirectionalLightComponent>("Directional Light");
    registry.registerComponent<PointLightComponent>("Point Light");
    registry.registerComponent<SpotLightComponent>("Spot Light");
    registry.registerComponent<SkyboxComponent>("Skybox");
}
