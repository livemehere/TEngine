#include "ComponentDrawerRegistry.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <string_view>
#include <utility>

#include <imgui.h>
#include <imgui_stdlib.h>

#include "../scene/ComponentTypeRegistry.h"

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
        .componentType = std::nullopt,
        .label = std::move(label),
        .predicate = std::move(predicate),
        .drawer = std::move(drawer)
    });
}

void ComponentDrawerRegistry::drawComponents(
    Scene &scene,
    Entity &entity,
    const ComponentTypeRegistry &componentTypes
) const {
    const ComponentTypeDescriptor *pendingRemoval = nullptr;
    constexpr ImGuiTreeNodeFlags componentFlags =
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_FramePadding |
            ImGuiTreeNodeFlags_AllowOverlap;

    const auto drawPanel = [&pendingRemoval](
        const std::string &label,
        const ComponentTypeDescriptor *descriptor,
        bool hasDirectComponent,
        const auto &drawContent
    ) {
        ImGui::PushID(label.c_str());
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
        const bool open = ImGui::TreeNodeEx(label.c_str(), componentFlags);
        ImGui::PopStyleVar();

        const bool canRemove = descriptor &&
                               descriptor->removable &&
                               hasDirectComponent;
        if (canRemove) {
            const ImGuiStyle &style = ImGui::GetStyle();
            const float buttonWidth = ImGui::CalcTextSize("...").x +
                                      style.FramePadding.x * 2.0f;
            const float buttonX = ImGui::GetWindowContentRegionMax().x -
                                  buttonWidth;

            ImGui::SameLine();
            ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), buttonX));
            if (ImGui::SmallButton("...")) {
                ImGui::OpenPopup("ComponentActions");
            }

            if (ImGui::BeginPopup("ComponentActions")) {
                if (ImGui::MenuItem("Remove Component")) {
                    pendingRemoval = descriptor;
                }
                ImGui::EndPopup();
            }
        }

        if (open) {
            ImGui::Spacing();
            drawContent();
            ImGui::TreePop();
        }
        ImGui::PopID();
    };

    for (const Entry &entry: entries) {
        if (!entry.predicate(scene, entity)) {
            continue;
        }

        const ComponentTypeDescriptor *descriptor = entry.componentType
                                                        ? componentTypes.find(*entry.componentType)
                                                        : nullptr;
        const bool hasDirectComponent = descriptor && descriptor->has(entity);
        drawPanel(entry.label, descriptor, hasDirectComponent, [&] {
            entry.drawer(scene, entity);
        });
    }

    for (const ComponentTypeDescriptor &componentType:
         componentTypes.getDescriptors()) {
        if (!componentType.has(entity)) {
            continue;
        }

        const bool hasCustomDrawer = std::ranges::any_of(
            entries,
            [&](const Entry &entry) {
                return entry.componentType &&
                       *entry.componentType == componentType.type;
            }
        );
        if (hasCustomDrawer) {
            continue;
        }

        drawPanel(componentType.name, &componentType, true, [&] {
            Component *component = componentType.get(entity);
            if (!component) {
                return;
            }

            ImGui::Checkbox("Enabled", &component->enabled);
            ImGui::TextDisabled("No custom inspector is registered.");
        });
    }

    if (pendingRemoval) {
        pendingRemoval->remove(entity);
    }
}

void ComponentDrawerRegistry::drawAddComponent(
    Entity &entity,
    const ComponentTypeRegistry &componentTypes
) {
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
    for (const ComponentTypeDescriptor &componentType:
         componentTypes.getDescriptors()) {
        if (!componentType.addable ||
            !containsCaseInsensitive(componentType.name, componentSearch)) {
            continue;
        }

        hasSearchResult = true;
        const bool canAdd = !componentType.has(entity);
        ImGui::BeginDisabled(!canAdd);

        if (ImGui::Selectable(componentType.name.c_str())) {
            componentType.addDefault(entity);
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
