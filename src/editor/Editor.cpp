#include "Editor.h"

#include <algorithm>
#include <format>
#include <limits>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>

#include "../core/Input.h"
#include "../scene/Scene.h"

Editor::Editor() {
    registerDefaultComponentDrawers(componentDrawers);
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
    componentDrawers.drawComponents(scene, *entity);
    componentDrawers.drawAddComponent(*entity);
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
            if (ImGui::MenuItem("Create Empty Entity")) {
                pendingEntityCreation = EntityCreationRequest{};
            }
            ImGui::EndPopup();
        }
    }
    ImGui::End();

    if (pendingMoveReq) {
        scene.moveEntity(pendingMoveReq->sourceId, pendingMoveReq->newParentId, pendingMoveReq->insertIndex);
        pendingMoveReq.reset();
    }

    if (pendingEntityCreation) {
        const std::optional<EntityId> parentId = pendingEntityCreation->parentId;
        Entity &created = scene.createEntity("Entity");

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

RenderExtent Editor::drawSceneView(GLuint textureId, bool isPlaying) {
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

    constexpr float playButtonWidth = 72.0f;
    const float toolbarWidth = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX() +
        std::max(0.0f, (toolbarWidth - playButtonWidth) * 0.5f)
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

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Create Child Entity")) {
            pendingEntityCreation = EntityCreationRequest{
                .parentId = entity.id
            };
        }
        ImGui::EndPopup();
    }

    if (hasChildren && opened) {
        drawSiblingList(scene, entity.id);
        ImGui::TreePop();
    }
}
