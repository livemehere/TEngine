#include "Window.h"
#include <imgui.h>

int main() {

    Window win;
    win.init();
    win.create_window(1280, 720, "Elementals", true);

    while (!win.should_close()) {
        win.before_update();

        // bool show_demo = true;
        // ImGui::ShowDemoWindow(&show_demo);

        ImGui::SetNextWindowSize(ImVec2(250,80), ImGuiCond_Once);
        ImGui::Begin("Debug");
        const Size size = win.get_size();
        ImGui::Text("size : %dx%d", size.w, size.h);
        ImGui::Text("buffer size : %dx%d", size.fb_w, size.fb_h);
        ImGui::End();


       // logic

        win.update();
    }

    return 0;
}