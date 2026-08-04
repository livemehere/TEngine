#include "ComponentDrawerRegistry.h"
#include "InstanceSelection.h"

#include <algorithm>
#include <cfloat>
#include <cstdint>
#include <format>
#include <span>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <imgui.h>

#include "../camera/CameraComponent.h"
#include "../rendering/Lights.h"
#include "../rendering/mesh/InstancedMeshRendererComponent.h"
#include "../rendering/mesh/MeshRendererComponent.h"
#include "../rendering/model/InstancedModelRendererComponent.h"
#include "../rendering/mesh/materials/PhongMaterial.h"
#include "../rendering/skybox/SkyboxComponent.h"
#include "../resources/ResourceManager.h"
#include "../scene/Scene.h"
#include "../scene/TransformComponent.h"

namespace {
    template<typename T>
    bool drawResourceSelector(
        const char *label,
        const T *&selectedResource,
        std::span<const ResourceEntry<T>> resources
    ) {
        std::string_view preview = selectedResource
                                       ? "Unregistered"
                                       : "None";
        for (const ResourceEntry<T> &entry: resources) {
            if (entry.resource == selectedResource) {
                preview = entry.name;
                break;
            }
        }

        bool changed = false;
        if (!ImGui::BeginCombo(label, preview.data())) {
            return false;
        }

        const bool noneSelected = selectedResource == nullptr;
        if (ImGui::Selectable("None", noneSelected)) {
            selectedResource = nullptr;
            changed = true;
        }
        if (noneSelected) {
            ImGui::SetItemDefaultFocus();
        }

        if (!resources.empty()) {
            ImGui::Separator();
        }

        for (const ResourceEntry<T> &entry: resources) {
            const bool selected = selectedResource == entry.resource;
            if (ImGui::Selectable(entry.name.c_str(), selected)) {
                selectedResource = entry.resource;
                changed = true;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
        return changed;
    }

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

    void drawInstanceCollection(
        EntityId entityId,
        InstanceRendererType rendererType,
        InstanceCollection &instances,
        std::optional<InstanceSelection> &selection
    ) {
        int requestedCount = static_cast<int>(instances.size());
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputInt("Instance Count", &requestedCount)) {
            requestedCount = std::clamp(requestedCount, 0, 100000);
            instances.resize(static_cast<std::size_t>(requestedCount));
        }

        const bool ownsSelection = selection &&
                                   selection->entityId == entityId &&
                                   selection->rendererType == rendererType;
        if (ownsSelection && !instances.find(selection->instanceId)) {
            selection.reset();
        }

        if (ImGui::Button("Add Instance")) {
            const InstanceId id = instances.add();
            selection = InstanceSelection{
                .entityId = entityId,
                .instanceId = id,
                .rendererType = rendererType
            };
        }

        ImGui::SameLine();
        const bool canDuplicate = selection &&
                                  selection->entityId == entityId &&
                                  selection->rendererType == rendererType &&
                                  instances.find(selection->instanceId);
        ImGui::BeginDisabled(!canDuplicate);
        if (ImGui::Button("Duplicate")) {
            const Transform transform =
                    instances.find(selection->instanceId)->transform;
            const InstanceId id = instances.add(transform);
            selection->instanceId = id;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::BeginDisabled(!canDuplicate);
        if (ImGui::Button("Remove")) {
            (void)instances.remove(selection->instanceId);
            selection.reset();
        }
        ImGui::EndDisabled();

        constexpr float listHeight = 140.0f;
        if (ImGui::BeginListBox("##Instances", ImVec2(-FLT_MIN, listHeight))) {
            std::vector<InstanceData> &items = instances.getItems();
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(items.size()));
            while (clipper.Step()) {
                for (int index = clipper.DisplayStart;
                     index < clipper.DisplayEnd;
                     ++index) {
                    const InstanceData &instance = items[index];
                    const bool selected = selection &&
                                          selection->entityId == entityId &&
                                          selection->rendererType == rendererType &&
                                          selection->instanceId == instance.id;
                    const std::string label = std::format(
                        "Instance {}##{}",
                        index,
                        instance.id
                    );
                    if (ImGui::Selectable(label.c_str(), selected)) {
                        selection = InstanceSelection{
                            .entityId = entityId,
                            .instanceId = instance.id,
                            .rendererType = rendererType
                        };
                    }
                }
            }
            ImGui::EndListBox();
        }

        if (!selection ||
            selection->entityId != entityId ||
            selection->rendererType != rendererType) {
            ImGui::TextDisabled("Select an instance to edit its transform.");
            return;
        }

        InstanceData *selected = instances.find(selection->instanceId);
        if (!selected) {
            selection.reset();
            return;
        }

        ImGui::SeparatorText("Selected Instance");
        drawVec3Control("position", selected->transform.position, 0.0f);
        drawVec3Control("rotation", selected->transform.rotation, 0.0f);
        drawVec3Control("scale", selected->transform.scale, 1.0f);
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

    std::vector<PhongMaterial *> collectPhongMaterials(
        const std::vector<MeshRendererComponent *> &components,
        ResourceManager &resources
    ) {
        std::vector<PhongMaterial *> result;
        std::unordered_set<PhongMaterial *> uniqueMaterials;

        for (const MeshRendererComponent *component : components) {
            Material *material = resources.findMutableMaterial(
                component->material
            );
            auto *phongMaterial = dynamic_cast<PhongMaterial *>(material);
            if (phongMaterial && uniqueMaterials.insert(phongMaterial).second) {
                result.push_back(phongMaterial);
            }
        }

        return result;
    }

    void drawEnvironmentMappingControls(
        const std::vector<MeshRendererComponent *> &components,
        ResourceManager &resources
    ) {
        const std::vector<PhongMaterial *> materials =
                collectPhongMaterials(components, resources);
        if (materials.empty()) {
            return;
        }

        const EnvironmentMappingMode firstMode =
                materials.front()->environmentMappingMode;
        const bool modeMixed = std::ranges::any_of(
            materials,
            [&](const PhongMaterial *material) {
                return material->environmentMappingMode != firstMode;
            }
        );

        ImGui::SeparatorText("Environment Mapping");

        constexpr const char *modeNames[] = {
            "Reflection",
            "Refraction"
        };
        const int mode = static_cast<int>(firstMode);
        if (ImGui::BeginCombo(
            "Mode",
            modeMixed ? "Mixed" : modeNames[mode]
        )) {
            for (int index = 0; index < IM_ARRAYSIZE(modeNames); ++index) {
                if (ImGui::Selectable(
                    modeNames[index],
                    !modeMixed && mode == index
                )) {
                    for (PhongMaterial *material : materials) {
                        material->environmentMappingMode =
                                static_cast<EnvironmentMappingMode>(index);
                    }
                }
            }
            ImGui::EndCombo();
        }

        const float firstStrength = materials.front()->environmentStrength;
        const bool strengthMixed = std::ranges::any_of(
            materials,
            [&](const PhongMaterial *material) {
                return material->environmentStrength != firstStrength;
            }
        );

        float strength = firstStrength;
        const char *strengthLabel = strengthMixed
                                        ? "Strength (Mixed)"
                                        : "Strength";
        if (ImGui::SliderFloat(
            strengthLabel,
            &strength,
            0.0f,
            1.0f,
            "%.2f"
        )) {
            for (PhongMaterial *material : materials) {
                material->environmentStrength = strength;
            }
        }

        if (!modeMixed && firstMode == EnvironmentMappingMode::Refraction) {
            const float firstRefractiveIndex =
                    materials.front()->refractiveIndex;
            const bool refractiveIndexMixed = std::ranges::any_of(
                materials,
                [&](const PhongMaterial *material) {
                    return material->refractiveIndex != firstRefractiveIndex;
                }
            );

            float refractiveIndex = firstRefractiveIndex;
            const char *refractiveIndexLabel = refractiveIndexMixed
                                                   ? "Refractive Index (Mixed)"
                                                   : "Refractive Index";
            if (ImGui::SliderFloat(
                refractiveIndexLabel,
                &refractiveIndex,
                1.0f,
                2.5f,
                "%.2f"
            )) {
                for (PhongMaterial *material : materials) {
                    material->refractiveIndex = refractiveIndex;
                }
            }
        }

        ImGui::TextDisabled(
            "%zu shared Phong material(s) in this subtree.",
            materials.size()
        );
    }

    void drawGeometryDebugControls(
        const std::vector<MeshRendererComponent *> &components
    ) {
        MeshRendererComponent &first = *components.front();
        const bool visibilityMixed = std::ranges::any_of(
            components,
            [&](const MeshRendererComponent *component) {
                return component->showVertexNormals !=
                       first.showVertexNormals;
            }
        );

        ImGui::SeparatorText("Geometry Debug");

        if (visibilityMixed) {
            ImGui::TextUnformatted("Vertex Normals: Mixed");
            if (ImGui::Button("Show All Normals")) {
                for (MeshRendererComponent *component : components) {
                    component->showVertexNormals = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Hide All Normals")) {
                for (MeshRendererComponent *component : components) {
                    component->showVertexNormals = false;
                }
            }
        } else {
            bool showVertexNormals = first.showVertexNormals;
            if (ImGui::Checkbox("Show Vertex Normals", &showVertexNormals)) {
                for (MeshRendererComponent *component : components) {
                    component->showVertexNormals = showVertexNormals;
                }
            }
        }

        const bool anyNormalsVisible = std::ranges::any_of(
            components,
            &MeshRendererComponent::showVertexNormals
        );
        if (anyNormalsVisible) {
            const float firstLength = first.vertexNormalLength;
            const bool lengthMixed = std::ranges::any_of(
                components,
                [&](const MeshRendererComponent *component) {
                    return component->vertexNormalLength != firstLength;
                }
            );

            float normalLength = firstLength;
            const char *lengthLabel = lengthMixed
                                          ? "Normal Length (Mixed)"
                                          : "Normal Length";
            if (ImGui::SliderFloat(
                lengthLabel,
                &normalLength,
                0.01f,
                2.0f,
                "%.2f"
            )) {
                for (MeshRendererComponent *component : components) {
                    component->vertexNormalLength = normalLength;
                }
            }
        }

        ImGui::TextDisabled(
            "Geometry shader output for %zu renderer(s).",
            components.size()
        );
    }

    void drawMeshRenderers(
        Scene &scene,
        Entity &entity,
        ResourceManager &resources
    ) {
        std::vector<MeshRendererComponent *> components =
                collectMeshRendererComponents(scene, entity.id);
        if (components.empty()) {
            return;
        }

        MeshRendererComponent &first = *components.front();
        if (MeshRendererComponent *direct =
                entity.tryGetComponent<MeshRendererComponent>()) {
            drawResourceSelector(
                "Mesh",
                direct->mesh,
                resources.getMeshResources()
            );
            drawResourceSelector(
                "Material",
                direct->material,
                resources.getMaterialResources()
            );
            ImGui::Separator();
        }

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

        drawGeometryDebugControls(components);
        drawEnvironmentMappingControls(components, resources);
    }
}

void registerDefaultComponentDrawers(
    ComponentDrawerRegistry &registry,
    ResourceManager &resources,
    std::optional<InstanceSelection> &instanceSelection
) {
    registry.registerDrawer<TransformComponent>(
        "Local Transform",
        [](Scene &, Entity &, TransformComponent &component) {
            Transform &transform = component.local;
            drawVec3Control("position", transform.position, 0.0f);
            drawVec3Control("rotation", transform.rotation, 0.0f);
            drawVec3Control("scale", transform.scale, 1.0f);
        }
    );

    registry.registerCustomDrawer<MeshRendererComponent>(
        "Mesh Renderer",
        [](Scene &scene, Entity &entity) {
            return !collectMeshRendererComponents(scene, entity.id).empty();
        },
        [&resources](Scene &scene, Entity &entity) {
            drawMeshRenderers(scene, entity, resources);
        }
    );

    registry.registerDrawer<InstancedMeshRendererComponent>(
        "Instanced Mesh Renderer",
        [&resources, &instanceSelection](
            Scene &,
            Entity &,
            InstancedMeshRendererComponent &component
        ) {
            ImGui::Checkbox("Enabled", &component.enabled);
            drawResourceSelector(
                "Mesh",
                component.mesh,
                resources.getMeshResources()
            );
            drawResourceSelector(
                "Material",
                component.material,
                resources.getMaterialResources()
            );
            drawInstanceCollection(
                component.getEntityId(),
                InstanceRendererType::Mesh,
                component.instances,
                instanceSelection
            );
            if (component.mesh) {
                const std::uint64_t trianglesPerInstance =
                        component.mesh->getTriangleCount();
                ImGui::Text(
                    "Triangles / Instance: %llu",
                    static_cast<unsigned long long>(trianglesPerInstance)
                );
                ImGui::Text(
                    "Submitted Triangles: %llu",
                    static_cast<unsigned long long>(
                        trianglesPerInstance * component.instances.size()
                    )
                );
            }
            if (component.material &&
                component.material->renderQueue != RenderQueueType::Opaque) {
                ImGui::TextColored(
                    ImVec4(1.0f, 0.55f, 0.25f, 1.0f),
                    "Only opaque materials are currently rendered."
                );
            }
        }
    );

    registry.registerDrawer<InstancedModelRendererComponent>(
        "Instanced Model Renderer",
        [&resources, &instanceSelection](
            Scene &,
            Entity &,
            InstancedModelRendererComponent &component
        ) {
            ImGui::Checkbox("Enabled", &component.enabled);
            drawResourceSelector(
                "Model",
                component.model,
                resources.getModelResources()
            );
            drawResourceSelector(
                "Fallback Material",
                component.fallbackMaterial,
                resources.getMaterialResources()
            );
            drawInstanceCollection(
                component.getEntityId(),
                InstanceRendererType::Model,
                component.instances,
                instanceSelection
            );
            if (component.model) {
                std::uint64_t trianglesPerInstance = 0;
                for (const ModelPart &part : component.model->parts) {
                    if (part.mesh) {
                        trianglesPerInstance += part.mesh->getTriangleCount();
                    }
                }
                ImGui::Text("Mesh Parts: %zu", component.model->parts.size());
                ImGui::Text(
                    "Triangles / Instance: %llu",
                    static_cast<unsigned long long>(trianglesPerInstance)
                );
                ImGui::Text(
                    "Submitted Triangles: %llu",
                    static_cast<unsigned long long>(
                        trianglesPerInstance * component.instances.size()
                    )
                );
            }
            ImGui::TextDisabled(
                "Each model mesh/material pair is rendered as one instanced draw."
            );
        }
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
}
