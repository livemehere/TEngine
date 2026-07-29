#include "Editor.h"

#include <print>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "../core/Scene.h"

namespace {
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

void Editor::draw(Scene &scene) {
    drawHierarchy(scene);
    drawInspector(scene);
}

void Editor::drawInspector(Scene &scene) {
    ImGui::Begin("Inspector");

    if (!selectedEntityId) {
        ImGui::TextDisabled("No entity selected");
        ImGui::End();
        return;
    }

    Entity *entity = scene.findEntity(*selectedEntityId);
    if (!entity) {
        selectedEntityId.reset();
        ImGui::End();
        return;
    }

    ImGui::Text("%s (id: %d)", entity->name.c_str(), static_cast<int>(entity->id));

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


    ImGui::End();
}

void Editor::drawHierarchy(Scene &scene) {
    ImGui::Begin("Hierarchy");
    drawSiblingList(scene, std::nullopt);
    ImGui::End();

    if (pendingMoveReq) {
        scene.moveEntity(pendingMoveReq->sourceId, pendingMoveReq->newParentId, pendingMoveReq->insertIndex);
        pendingMoveReq.reset();
    }
}

void Editor::drawSiblingList(Scene &scene, std::optional<EntityId> id) {
    auto siblings = scene.getChildren(id);

    drawInsertionSlot(scene, id, 0);

    for (size_t i=0; i< siblings.size(); i++) {
        drawEntityNode(scene, *siblings[i]);
        drawInsertionSlot(scene, id, i+1);
    }

}

void Editor::drawInsertionSlot(Scene &scene, std::optional<EntityId> id, size_t insertIndex) {
    auto slotId = std::format("##DropSlot_{}_{}", id.value_or(std::numeric_limits<EntityId>::max()), insertIndex);

    ImGui::InvisibleButton(slotId.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 2.0f));

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
           EntityId sourceId = *static_cast<EntityId*>(payload->Data);
            pendingMoveReq = {
                .sourceId = sourceId,
                .newParentId = id,
                .insertIndex = insertIndex
            };
        }
    }

    ImGui::EndDragDropTarget();
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
            EntityId sourceId = *static_cast<EntityId *>(payload->Data);
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
