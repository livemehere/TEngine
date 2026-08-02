#include "ComponentDrawerRegistry.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <string_view>
#include <utility>

#include <imgui.h>
#include <imgui_stdlib.h>

namespace {
    bool containsCaseInsensitive(std::string_view text, std::string_view query) {
        return std::ranges::search(
            text,
            query,
            [](char left, char right) {
                return std::tolower(static_cast<unsigned char>(left)) ==
                       std::tolower(static_cast<unsigned char>(right));
            }
        ).begin() != text.end();
    }
}

void ComponentDrawerRegistry::registerCustomDrawer(
    std::string label,
    Predicate predicate,
    Drawer drawer
) {
    entries.push_back({
        .label = std::move(label),
        .predicate = std::move(predicate),
        .drawer = std::move(drawer)
    });
}

void ComponentDrawerRegistry::drawComponents(Scene &scene, Entity &entity) const {
    constexpr ImGuiTreeNodeFlags componentFlags =
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_FramePadding;

    for (const Entry &entry: entries) {
        if (!entry.predicate(scene, entity)) {
            continue;
        }

        ImGui::PushID(entry.label.c_str());
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
        const bool open = ImGui::TreeNodeEx(entry.label.c_str(), componentFlags);
        ImGui::PopStyleVar();

        if (open) {
            ImGui::Spacing();
            entry.drawer(scene, entity);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

void ComponentDrawerRegistry::drawAddComponent(Entity &entity) {
    ImGui::Spacing();
    ImGui::Separator();

    if (ImGui::Button("Add Component", ImVec2(-FLT_MIN, 0.0f))) {
        componentSearch.clear();
        ImGui::OpenPopup("AddComponentPopup");
    }

    ImGui::SetNextWindowSize(ImVec2(280.0f, 0.0f), ImGuiCond_Appearing);
    if (!ImGui::BeginPopup("AddComponentPopup")) {
        return;
    }

    if (ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere();
    }

    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint(
        "##ComponentSearch",
        "Search components...",
        &componentSearch
    );
    ImGui::Separator();

    bool hasSearchResult = false;
    for (const FactoryEntry &factory: factories) {
        if (!containsCaseInsensitive(factory.label, componentSearch)) {
            continue;
        }

        hasSearchResult = true;
        const bool canAdd = factory.canAdd(entity);
        ImGui::BeginDisabled(!canAdd);

        if (ImGui::Selectable(factory.label.c_str())) {
            factory.add(entity);
            componentSearch.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndDisabled();
        if (!canAdd && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("This entity already has this component.");
        }
    }

    if (!hasSearchResult) {
        ImGui::TextDisabled("No components found");
    }

    ImGui::EndPopup();
}
