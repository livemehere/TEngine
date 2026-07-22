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
#include "rendering/mesh/materials/LitMaterial.h"
#include "rendering/mesh/materials/UnlitMaterial.h"
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
        Texture2D whiteTexture{1,1,pixels};
        Texture2D boxTexture("textures/box.png");
        Texture2D boxSpecularMapTexture("textures/box_specular_map.png");

        /* lit */
        Shader litShader{"shaders/basic.vert", "shaders/lit.frag"};
        litShader.bindUniformBlock("CameraData", UniformBinding::Camera);
        litShader.bindUniformBlock("LightsData", UniformBinding::Lights);
        LitMaterial white{litShader,whiteTexture};
        LitMaterial boxMaterial{litShader,boxTexture, {0.2f,0.2f,0.2f,1.0f}};
        boxMaterial.specularTexture = &boxSpecularMapTexture;

        /* unlit */
        Shader unlitShader{"shaders/basic.vert", "shaders/unlit.frag"};
        UnlitMaterial unlitMaterial{unlitShader,whiteTexture};

        // ground
        scene.meshObjects.push_back({
            .transform = {
                .position = {0.0f,0.0f,0.0f},
                .rotation = {-90.0f,0.0f,0.0f},
                .scale = {5.0f,5.0f,5.0f},
            },
            .mesh = &resourceManager.getPlaneMesh(),
            .material = &white,
        });

        // cube
        scene.meshObjects.push_back({
           .transform = {
               .position = {0.0f,1.1f,0.0f},
               .rotation = {0.0f,0.0f,0.0f},
               .scale = {1.0f,1.0f,1.0f},
           },
           .mesh = &resourceManager.getCubeMesh(),
           .material = &boxMaterial,
       });

        scene.ambientLight.intensity = 0.1f;
        // light
        scene.pointLights.push_back({
            .position = {1.5f, 1.0f,0.0f},
            .range = 5.0f,
            .color = {1.0f,1.0f,1.0f},
            .intensity = 1.0f,
        });

        scene.directionalLights.push_back({
            .direction = glm::vec3{0.0f, -1.0f, 0.0f},
            .color = glm::vec3{1.0f},
            .intensity = 0.1f
        });

        scene.spotLights.push_back({
            .direction = glm::vec3{0.0f, -1.0f, 0.0f},
            .position = glm::vec3{0.0f, 4.0f, 0.0},
            .range = 10.0f,
            .color = glm::vec3{1.0f,1.0f, 0.0f},
            .intensity = 10.0f,
        });

       //  scene.pointLights.push_back({
       //      .position = {-1.5f, 1.0f,0.0f},
       //      .range = 5.0f,
       //      .color = {1.0f,0.0f,0.0f},
       //      .intensity = 1.0f,
       //    });
       //
       //  scene.pointLights.push_back({
       //     .position = {0.0f, 1.0f,1.0f},
       //     .range = 5.0f,
       //     .color = {0.0f,1.0f,0.0f},
       //     .intensity = 1.0f,
       // });
        /* ---------- */

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

            ImGui::SeparatorText("Ambient Light");
            ImGui::DragFloat("ambientLight.intensity", &scene.ambientLight.intensity, 0.1f);

            ImGui::SeparatorText("Directional Light");
            ImGui::DragFloat3("directionalLight.direction", glm::value_ptr(scene.directionalLights[0].direction), 0.1f);
            ImGui::DragFloat("directionalLight.intensity", &scene.directionalLights[0].intensity, 0.1f);

            ImGui::SeparatorText("Point Light");
            ImGui::DragFloat("pointLight.intensity", &scene.pointLights[0].intensity, 0.1f);
            ImGui::DragFloat3("pointLight.position", glm::value_ptr(scene.pointLights[0].position), 0.1f);

            ImGui::SeparatorText("Spot Light");
            ImGui::DragFloat("spotLight.intensity", &scene.spotLights[0].intensity, 0.1f);
            ImGui::DragFloat("spotLight.range", &scene.spotLights[0].range, 0.1f);
            ImGui::DragFloat("spotLight.innerAngle", &scene.spotLights[0].innerAngle, 0.1f);
            ImGui::DragFloat("spotLight.outerAngle", &scene.spotLights[0].outerAngle, 0.1f);
            ImGui::DragFloat3("spotLight.position", glm::value_ptr(scene.spotLights[0].position), 0.1f);
            ImGui::DragFloat3("spotLight.color", glm::value_ptr(scene.spotLights[0].color), 0.1f);

            ImGui::SeparatorText("LitMaterial");
            ImGui::DragFloat("shininess", &boxMaterial.shininess, 0.1f);
            ImGui::DragFloat("specularStrength", &boxMaterial.specularStrength, 0.1f);

            ImGui::End();

            win.update();
        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}
