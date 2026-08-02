#include "EditorTheme.h"

#include <imgui.h>

namespace {
    constexpr ImVec4 color(
        float red,
        float green,
        float blue,
        float alpha = 1.0f
    ) {
        return {red, green, blue, alpha};
    }
}

void EditorTheme::applyModernDark() {
    ImGui::StyleColorsDark();

    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowPadding = {10.0f, 10.0f};
    style.FramePadding = {8.0f, 5.0f};
    style.CellPadding = {6.0f, 5.0f};
    style.ItemSpacing = {8.0f, 6.0f};
    style.ItemInnerSpacing = {6.0f, 4.0f};
    style.TouchExtraPadding = {0.0f, 0.0f};
    style.IndentSpacing = 18.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
    style.TabBarBorderSize = 1.0f;

    style.WindowRounding = 6.0f;
    style.ChildRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    style.WindowTitleAlign = {0.02f, 0.5f};
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.SeparatorTextBorderSize = 1.0f;
    style.SeparatorTextAlign = {0.0f, 0.5f};
    style.SeparatorTextPadding = {0.0f, 6.0f};

    ImVec4 *colors = style.Colors;
    colors[ImGuiCol_Text] = color(0.88f, 0.90f, 0.94f);
    colors[ImGuiCol_TextDisabled] = color(0.47f, 0.50f, 0.57f);
    colors[ImGuiCol_WindowBg] = color(0.055f, 0.063f, 0.078f);
    colors[ImGuiCol_ChildBg] = color(0.055f, 0.063f, 0.078f);
    colors[ImGuiCol_PopupBg] = color(0.075f, 0.086f, 0.105f, 0.98f);
    colors[ImGuiCol_Border] = color(0.16f, 0.18f, 0.22f);
    colors[ImGuiCol_BorderShadow] = color(0.0f, 0.0f, 0.0f, 0.0f);

    colors[ImGuiCol_FrameBg] = color(0.095f, 0.108f, 0.132f);
    colors[ImGuiCol_FrameBgHovered] = color(0.13f, 0.16f, 0.20f);
    colors[ImGuiCol_FrameBgActive] = color(0.16f, 0.20f, 0.26f);
    colors[ImGuiCol_TitleBg] = color(0.045f, 0.052f, 0.065f);
    colors[ImGuiCol_TitleBgActive] = color(0.065f, 0.075f, 0.093f);
    colors[ImGuiCol_TitleBgCollapsed] = color(0.045f, 0.052f, 0.065f);
    colors[ImGuiCol_MenuBarBg] = color(0.060f, 0.068f, 0.084f);

    colors[ImGuiCol_ScrollbarBg] = color(0.045f, 0.052f, 0.065f);
    colors[ImGuiCol_ScrollbarGrab] = color(0.18f, 0.20f, 0.24f);
    colors[ImGuiCol_ScrollbarGrabHovered] = color(0.25f, 0.28f, 0.34f);
    colors[ImGuiCol_ScrollbarGrabActive] = color(0.32f, 0.36f, 0.43f);

    colors[ImGuiCol_CheckMark] = color(0.29f, 0.62f, 1.0f);
    colors[ImGuiCol_SliderGrab] = color(0.29f, 0.62f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = color(0.43f, 0.71f, 1.0f);
    colors[ImGuiCol_Button] = color(0.12f, 0.15f, 0.19f);
    colors[ImGuiCol_ButtonHovered] = color(0.19f, 0.39f, 0.64f);
    colors[ImGuiCol_ButtonActive] = color(0.22f, 0.48f, 0.78f);

    colors[ImGuiCol_Header] = color(0.12f, 0.15f, 0.19f);
    colors[ImGuiCol_HeaderHovered] = color(0.17f, 0.32f, 0.50f);
    colors[ImGuiCol_HeaderActive] = color(0.20f, 0.41f, 0.66f);
    colors[ImGuiCol_Separator] = color(0.16f, 0.18f, 0.22f);
    colors[ImGuiCol_SeparatorHovered] = color(0.29f, 0.62f, 1.0f);
    colors[ImGuiCol_SeparatorActive] = color(0.43f, 0.71f, 1.0f);
    colors[ImGuiCol_ResizeGrip] = color(0.29f, 0.62f, 1.0f, 0.16f);
    colors[ImGuiCol_ResizeGripHovered] = color(0.29f, 0.62f, 1.0f, 0.55f);
    colors[ImGuiCol_ResizeGripActive] = color(0.29f, 0.62f, 1.0f, 0.85f);

    colors[ImGuiCol_Tab] = color(0.075f, 0.086f, 0.105f);
    colors[ImGuiCol_TabHovered] = color(0.18f, 0.38f, 0.62f);
    colors[ImGuiCol_TabSelected] = color(0.15f, 0.29f, 0.46f);
    colors[ImGuiCol_TabSelectedOverline] = color(0.29f, 0.62f, 1.0f);
    colors[ImGuiCol_TabDimmed] = color(0.055f, 0.063f, 0.078f);
    colors[ImGuiCol_TabDimmedSelected] = color(0.10f, 0.16f, 0.23f);

    colors[ImGuiCol_DockingPreview] = color(0.29f, 0.62f, 1.0f, 0.65f);
    colors[ImGuiCol_DockingEmptyBg] = color(0.035f, 0.040f, 0.050f);
    colors[ImGuiCol_TableHeaderBg] = color(0.085f, 0.097f, 0.118f);
    colors[ImGuiCol_TableBorderStrong] = color(0.16f, 0.18f, 0.22f);
    colors[ImGuiCol_TableBorderLight] = color(0.12f, 0.14f, 0.17f);
    colors[ImGuiCol_TableRowBg] = color(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TableRowBgAlt] = color(1.0f, 1.0f, 1.0f, 0.018f);

    colors[ImGuiCol_TextSelectedBg] = color(0.29f, 0.62f, 1.0f, 0.30f);
    colors[ImGuiCol_DragDropTarget] = color(0.43f, 0.71f, 1.0f);
    colors[ImGuiCol_NavCursor] = color(0.43f, 0.71f, 1.0f);
    colors[ImGuiCol_ModalWindowDimBg] = color(0.0f, 0.0f, 0.0f, 0.55f);
}
