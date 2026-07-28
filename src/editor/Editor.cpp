#include "Editor.h"

#include <print>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "../core/Scene.h"

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
    ImGui::DragFloat3("position", glm::value_ptr(entity->localTransform.position), 0.1f);
    ImGui::DragFloat3("rotation", glm::value_ptr(entity->localTransform.rotation), 0.1f);
    ImGui::DragFloat3("scale", glm::value_ptr(entity->localTransform.scale), 0.1f);

    ImGui::End();
}
