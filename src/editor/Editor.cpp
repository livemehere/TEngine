#include "Editor.h"

#include <algorithm>
#include <cfloat>
#include <format>
#include <limits>
#include <vector>

#include <imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

#include "../core/Input.h"
#include "../scene/Scene.h"

namespace {
    void collectMeshRendererComponents(
        Scene &scene,
        EntityId rootId,
        std::vector<MeshRendererComponent *> &result
    ) {
        Entity *entity = scene.findEntity(rootId);
        if (!entity) {
            return;
        }

        if (entity->meshRenderComponent) {
            result.push_back(&*entity->meshRenderComponent);
        }

        for (const Entity *child: scene.getChildren(rootId)) {
            collectMeshRendererComponents(scene, child->id, result);
        }
    }

    bool drawVec3Control(const char *label, glm::vec3 &value, float resetValue, float step = 0.1f) {
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

                float buttonSize = ImGui::GetFrameHeight();
                if (ImGui::Button(axisLabel, ImVec2(buttonSize + 3.0f, buttonSize))) {
                    axisValue = resetValue;
                    changed = true;
                }

                ImGui::PopStyleColor(3);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-FLT_MIN);

                changed |= ImGui::DragFloat("##Value", &axisValue, step, 0.0f, 0.0f, "%.2f");

                ImGui::PopStyleVar();
                ImGui::PopID();
            };

            drawAxis(
                1,
                "X",
                value.x,
                ImVec4(0.80f, 0.10f, 0.15f, 1.0f),
                ImVec4(0.90f, 0.20f, 0.25f, 1.0f),
                ImVec4(0.70f, 0.05f, 0.10f, 1.0f)
            );

            drawAxis(
                2,
                "Y",
                value.y,
                ImVec4(0.20f, 0.70f, 0.20f, 1.0f),
                ImVec4(0.30f, 0.80f, 0.30f, 1.0f),
                ImVec4(0.10f, 0.60f, 0.10f, 1.0f)
            );

            drawAxis(
                3,
                "Z",
                value.z,
                ImVec4(0.10f, 0.25f, 0.80f, 1.0f),
                ImVec4(0.20f, 0.35f, 0.90f, 1.0f),
                ImVec4(0.05f, 0.15f, 0.70f, 1.0f)
            );

            ImGui::EndTable();
        }
        ImGui::PopID();
        return changed;
    }
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

void Editor::draw(Scene &scene, const WindowSize& windowSize, const MouseState& mouseState) {
    drawHierarchy(scene);
    drawInspector(scene);
    drawDebug(scene, windowSize, mouseState);
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

    ImGui::Text(
        "%s (id: %llu)",
        entity->name.c_str(),
        static_cast<unsigned long long>(entity->id)
    );

    /* transform start */
    constexpr ImGuiTreeNodeFlags componentFlags =
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_FramePadding;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
    bool transformOpen = ImGui::TreeNodeEx("LocalTransform", componentFlags);
    ImGui::PopStyleVar();

    if (transformOpen) {
        ImGui::Spacing();

        bool positionChanged = drawVec3Control("position", entity->localTransform.position, 0.0f, 0.1f);
        bool rotationChanged = drawVec3Control("rotation", entity->localTransform.rotation, 0.0f, 0.1f);
        bool scaleChanged = drawVec3Control("scale", entity->localTransform.scale, 1.0f, 0.1f);

        if (positionChanged || rotationChanged || scaleChanged) {
            // TODO: dirty, undo
        }
        ImGui::TreePop();
    }
    /* transform end */

    std::vector<MeshRendererComponent *> meshRenderers;
    collectMeshRendererComponents(scene, entity->id, meshRenderers);

    if (!meshRenderers.empty()) {
        const std::string componentLabel = std::format(
            "MeshRenderer ({})###MeshRenderer",
            meshRenderers.size()
        );

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
        const bool meshRendererOpen = ImGui::TreeNodeEx(componentLabel.c_str(), componentFlags);
        ImGui::PopStyleVar();

        if (meshRendererOpen) {
            ImGui::Spacing();

            const MeshRendererComponent &first = *meshRenderers.front();

            const bool enabledMixed = std::ranges::any_of(
                meshRenderers,
                [&](const MeshRendererComponent *component) {
                    return component->outlineEnabled != first.outlineEnabled;
                }
            );

            if (enabledMixed) {
                ImGui::TextUnformatted("Persistent Outline: Mixed");

                if (ImGui::Button("Enable All")) {
                    for (MeshRendererComponent *component: meshRenderers) {
                        component->outlineEnabled = true;
                    }
                }

                ImGui::SameLine();
                if (ImGui::Button("Disable All")) {
                    for (MeshRendererComponent *component: meshRenderers) {
                        component->outlineEnabled = false;
                    }
                }
            } else {
                bool outlineEnabled = first.outlineEnabled;
                if (ImGui::Checkbox("Persistent Outline", &outlineEnabled)) {
                    for (MeshRendererComponent *component: meshRenderers) {
                        component->outlineEnabled = outlineEnabled;
                    }
                }
            }

            ImGui::TextDisabled(
                "%zu renderer(s) in this subtree. Selection highlight is independent.",
                meshRenderers.size()
            );

            constexpr const char *outlineModeNames[] = {
                "Normal Extrusion",
                "Scale From Pivot"
            };

            const bool modeMixed = std::ranges::any_of(
                meshRenderers,
                [&](const MeshRendererComponent *component) {
                    return component->outlineMode != first.outlineMode;
                }
            );
            const int outlineMode = static_cast<int>(first.outlineMode);
            const char *outlineModePreview = modeMixed
                                                 ? "Mixed"
                                                 : outlineModeNames[outlineMode];

            if (ImGui::BeginCombo("Outline Mode", outlineModePreview)) {
                for (int i = 0; i < IM_ARRAYSIZE(outlineModeNames); ++i) {
                    if (ImGui::Selectable(
                        outlineModeNames[i],
                        !modeMixed && outlineMode == i
                    )) {
                        for (MeshRendererComponent *component: meshRenderers) {
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
                meshRenderers,
                [&](const MeshRendererComponent *component) {
                    return component->outlineVisibility != first.outlineVisibility;
                }
            );
            const int outlineVisibility = static_cast<int>(first.outlineVisibility);
            const char *outlineVisibilityPreview = visibilityMixed
                                                       ? "Mixed"
                                                       : outlineVisibilityNames[outlineVisibility];

            if (ImGui::BeginCombo("Outline Visibility", outlineVisibilityPreview)) {
                for (int i = 0; i < IM_ARRAYSIZE(outlineVisibilityNames); ++i) {
                    if (ImGui::Selectable(
                        outlineVisibilityNames[i],
                        !visibilityMixed && outlineVisibility == i
                    )) {
                        for (MeshRendererComponent *component: meshRenderers) {
                            component->outlineVisibility = static_cast<OutlineVisibility>(i);
                        }
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::TreePop();
        }
    }

    if (entity->skyboxComponent) {
        constexpr ImGuiTreeNodeFlags skyboxFlags =
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_FramePadding;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
        const bool skyboxOpen = ImGui::TreeNodeEx("Skybox", skyboxFlags);
        ImGui::PopStyleVar();

        if (skyboxOpen) {
            ImGui::Checkbox("Enabled", &entity->skyboxComponent->enabled);
            ImGui::TextDisabled("The skybox follows the camera; entity transform is ignored.");
            ImGui::TreePop();
        }
    }

}

void Editor::drawDebug(Scene &scene, const WindowSize& windowSize, const MouseState& mouseState) {
    if (ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::SeparatorText("Performance");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        ImGui::SeparatorText("Window");
        ImGui::Text("Logical: %d x %d", windowSize.w, windowSize.h);
        ImGui::Text("Framebuffer: %d x %d", windowSize.fb_w, windowSize.fb_h);

        ImGui::SeparatorText("Input");
        ImGui::Text("Cursor: %.2f, %.2f", mouseState.screenX, mouseState.screenY);
        ImGui::Text("Delta: %.2f, %.2f", mouseState.deltaX, mouseState.deltaY);
        ImGui::Text("Left: %s", mouseState.leftBtnDown ? "Pressed" : "None");
        ImGui::Text("Right: %s", mouseState.rightBtnDown ? "Pressed" : "None");

        ImGui::SeparatorText("Camera");
        ImGui::DragFloat3("Position##Camera", glm::value_ptr(scene.camera.transform.position), 1.0f);
        ImGui::DragFloat3("Rotation##Camera", glm::value_ptr(scene.camera.transform.rotation), 1.0f);

        const bool isPerspective = std::holds_alternative<PerspectiveProjection>(scene.camera.projection);
        if (ImGui::BeginCombo("Projection", isPerspective ? "Perspective" : "Orthographic")) {
            if (ImGui::Selectable("Perspective", isPerspective)) {
                scene.camera.projection = PerspectiveProjection{};
            }
            if (ImGui::Selectable("Orthographic", !isPerspective)) {
                scene.camera.projection = OrthoGraphicProjection{};
            }
            ImGui::EndCombo();
        }
    }
    ImGui::End();
}

void Editor::drawHierarchy(Scene &scene) {
    if (ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse)) {
        drawSiblingList(scene, std::nullopt);
    }
    ImGui::End();

    if (pendingMoveReq) {
        scene.moveEntity(pendingMoveReq->sourceId, pendingMoveReq->newParentId, pendingMoveReq->insertIndex);
        pendingMoveReq.reset();
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

RenderExtent Editor::drawSceneView(GLuint textureId) {
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

    const ImVec2 available = ImGui::GetContentRegionAvail();
    const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;

    const RenderExtent extent{
        .width = static_cast<int>(available.x * scale.x),
        .height = static_cast<int>(available.y * scale.y)
    };

    if (extent.width > 0 && extent.height > 0) {
        ImGui::Image(textureId, available, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    }

    ImGui::End();
    return extent;
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


    if (hasChildren && opened) {
        drawSiblingList(scene, entity.id);
        ImGui::TreePop();
    }
}
