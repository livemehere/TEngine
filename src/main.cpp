#include "common.h"

#include <iostream>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "Input.h"
#include "Window.h"
#include "World.h"
#include "camera/Camera.h"
#include "camera/FreeLookCameraController.h"


int main() {

    try {
        Window win;
        win.init();
        win.create_window(1280, 720, "Elementals", true);

        Input input{win};
        Camera camera;
        FreeLookCameraController controller;
        World world;

        float lastFrameTime = static_cast<float>(glfwGetTime());
        while (!win.should_close()) {
            win.before_update();

            float currentFrameTime = static_cast<float>(glfwGetTime());
            float dt = (currentFrameTime - lastFrameTime);
            // dt = std::min(dt, 0.1f); // prevent spark when replay after paused(debugging..)
            lastFrameTime = currentFrameTime;

            const WindowSize& size = win.get_size();
            const MouseState& mouseState = input.getMouseState();


            /* update */
            input.update();
            controller.update(camera, input, dt);
            camera.update();
            world.update(camera.getViewMatrix(), camera.getProjectionMatrix(size));

            /* render */
            world.render();

            ImGui::SetNextWindowSize(ImVec2(250,150), ImGuiCond_Once);
            ImGui::Begin("Debug");
            ImGui::Text("FPS %.1f FPS", ImGui::GetIO().Framerate);
            ImGui::Text("size : %dx%d", size.w, size.h);
            ImGui::Text("buffer size : %dx%d", size.fb_w, size.fb_h);
            ImGui::Text("screen pos : %.2fx%.2f", mouseState.screenX,mouseState.screenY);
            ImGui::Text("cursor delta : %.2fx%.2f", mouseState.deltaX,mouseState.deltaY);

            auto leftStateStr = std::format("Left : {}", mouseState.leftBtnDown ? "Pressed" : "NONE");
            ImGui::Text(leftStateStr.c_str());

            auto rightStateStr = std::format("Right : {}", mouseState.rightBtnDown ? "Pressed" : "NONE");
            ImGui::Text(rightStateStr.c_str());

            ImGui::End();

            ImGui::Begin("Camera");
            ImGui::DragFloat3("position", glm::value_ptr(camera.transform.position), 1.0f);
            ImGui::DragFloat3("rotation", glm::value_ptr(camera.transform.rotation), 1.0f);

            static bool is3DMode = true;
            if (ImGui::Selectable(is3DMode ? "3D" : "2D", is3DMode)) {
                is3DMode = !is3DMode;
                if (is3DMode) {
                    camera.projection = PerspectiveProjection{};
                }else {
                    camera.projection = OrthoGraphicProjection{};
                }
            }

            ImGui::End();


            win.update();
        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}
