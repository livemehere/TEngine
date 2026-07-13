#include <iostream>

#include "Window.h"
#include "World.h"

int main() {

    try {
        Window win;
        win.init();
        win.create_window(1280, 720, "Elementals", true);

        World world;

        while (!win.should_close()) {
            win.before_update();

            world.update();
            world.render();

            ImGui::SetNextWindowSize(ImVec2(250,80), ImGuiCond_Once);
            ImGui::Begin("Debug");
            const Size size = win.get_size();
            ImGui::Text("size : %dx%d", size.w, size.h);
            ImGui::Text("buffer size : %dx%d", size.fb_w, size.fb_h);
            ImGui::End();

            win.update();
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
