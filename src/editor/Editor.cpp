#include "Editor.h"

#include <print>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "../core/Scene.h"

namespace {
    bool drawVec3Control(const char *label, glm::vec3 &value, float resetValue, float speed = 0.1f) {
        bool changed = false;

        ImGui::PushID(label);

        // TODO: 플레그 의미?
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
        }


    }
}

void Editor::draw(Scene &scene) {
    drawHierarchy(scene);
    drawInspector(scene);
}

void Editor::drawHierarchy(Scene &scene) {
    ImGui::Begin("Hierarchy");

    for (const Entity &entity: scene.getEntities()) {
        bool selected = entity.id == selectedEntityId;
        if (ImGui::Selectable(entity.name.c_str(), selected)) {
            selectedEntityId = entity.id;
            std::println("select {}", *selectedEntityId);
        }
    }

    ImGui::End();
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

    ImGui::Text("%s", entity->name.c_str());
    if (ImGui::CollapsingHeader("LocalTransform", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3("position", glm::value_ptr(entity->localTransform.position), 0.1f);
        ImGui::DragFloat3("rotation", glm::value_ptr(entity->localTransform.rotation), 0.1f);
        ImGui::DragFloat3("scale", glm::value_ptr(entity->localTransform.scale), 0.1f);
    }

    ImGui::End();
}
