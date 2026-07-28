#include "Editor.h"

#include <imgui.h>

#include "../core/Scene.h"

void Editor::draw(Scene &scene) {
    drawHierarchy(scene);
}

void Editor::drawHierarchy(Scene &scene) {
    ImGui::Begin("Hierarchy");

    for (const Entity& entity : scene.getEntities()) {
        bool selected = entity.id == selectedEntityId;
        if (ImGui::Selectable(entity.name.c_str(),selected)) {
            selectedEntityId = entity.id;
            std::printf("select %d", selectedEntityId);
        }
    }

    ImGui::End();
}

void Editor::drawInspector(Scene &scene) {
    ImGui::Begin("Inspector");

    ImGui::End();
}

