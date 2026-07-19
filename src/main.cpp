#include <iostream>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "core/Input.h"
#include "core/Window.h"
#include "Scene.h"
#include "camera/Camera.h"
#include "camera/FreeLookCameraController.h"
#include "graphics/Texture2D.h"
#include "rendering/Renderer.h"
#include "rendering/mesh/materials/BasicMaterial.h"
#include "resources/ResourceManager.h"


int main() {

    try {
        Window win;
        win.init();
        win.create_window(1280, 720, "T-Engine Sandbox", true);

        Input input{win};
        ResourceManager resourceManager;
        Scene scene;
        Renderer renderer;
        FreeLookCameraController cameraController;

        /* TEMP SETUP */
        scene.camera.transform.position.x = 3.0f;
        scene.camera.transform.position.z = 3.0f;
        scene.camera.transform.position.y = 3.0f;
        scene.camera.lookAt(glm::vec3(0.0f));
        // scene.camera.projection = OrthoGraphicProjection{};

        std::array<uint8_t,4> pixels = {
            255,255,255,255,
        };
        Shader basicShader{"shaders/basic.vert", "shaders/basic.frag"};
        Shader lightShader{"shaders/basic.vert", "shaders/light.frag"};
        Texture2D whiteTexture{1,1,pixels};
        BasicMaterial lightMaterial{lightShader,whiteTexture};
        BasicMaterial green{basicShader,whiteTexture, {0.2f, 0.5f, 0.1f,1.0f}};
        BasicMaterial orange{basicShader,whiteTexture, {1.0f, 0.5f, 0.3f,1.0f}};

        // ground
        scene.meshObjects.push_back({
            .transform = {
                .position = {0.0f,0.0f,0.0f},
                .rotation = {-90.0f,0.0f,0.0f},
                .scale = {5.0f,5.0f,5.0f},
            },
            .mesh = &resourceManager.getPlaneMesh(),
            .material = &green,
        });

        // cube
        scene.meshObjects.push_back({
           .transform = {
               .position = {0.0f,1.1f,0.0f},
               .rotation = {0.0f,0.0f,0.0f},
               .scale = {1.0f,1.0f,1.0f},
           },
           .mesh = &resourceManager.getCubeMesh(),
           .material = &orange,
       });

        // light
        // scene.pointLights.push_back({
        //     .position = {1.0f, 1.0f,0.0f},
        //     .color = {1.0f,1.0f,1.0f},
        //     .intensity = 1.0f,
        //     .range = 10.0f
        // });
        /* ---------- */

        glm::vec3 lightPos(1.0f, 1.0f, 0.0f);

        auto lastFrameTime = static_cast<float>(glfwGetTime());
        while (!win.should_close()) {

            /* currentFrame info */
            float currentFrameTime = static_cast<float>(glfwGetTime());
            float dt = (currentFrameTime - lastFrameTime);
            // dt = std::min(dt, 0.1f); // prevent spark when replay after paused(debugging..)
            lastFrameTime = currentFrameTime;
            const WindowSize& size = win.get_size();
            const MouseState& mouseState = input.getMouseState();

            /* update */
            win.pollEvents();
            input.update();
            cameraController.update(scene.camera, input, dt);

            scene.update(dt);
            renderer.beginFrame(scene, size);
            renderer.render(scene);
            renderer.endFrame();

            /* render debug */
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

            ImGui::Begin("Properties");
            ImGui::SeparatorText("Camera");
            ImGui::DragFloat3("position", glm::value_ptr(scene.camera.transform.position), 1.0f);
            ImGui::DragFloat3("rotation", glm::value_ptr(scene.camera.transform.rotation), 1.0f);

            static bool is3DMode = true;
            if (ImGui::Selectable(is3DMode ? "3D" : "2D", is3DMode)) {
                is3DMode = !is3DMode;
                if (is3DMode) {
                    scene.camera.projection = PerspectiveProjection{};
                }else {
                    scene.camera.projection = OrthoGraphicProjection{};
                }
            }

            ImGui::SeparatorText("Light");
            ImGui::DragFloat("ambientLight.intensity", &scene.ambientLight.intensity, 0.1f);

            ImGui::End();
            win.update();
        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}
