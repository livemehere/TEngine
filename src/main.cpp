#include "Window.h"
#include <imgui.h>

int main() {

    Window win;
    win.init();
    win.create_window(1280, 720, "Elementals", true);

    while (!win.should_close()) {
        win.before_update();

        bool show_demo = true;
        ImGui::ShowDemoWindow(&show_demo);

       // logic

        win.update();
    }

    return 0;
}