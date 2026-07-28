#include "Application.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "../camera/FreeLookCameraController.h"

void Application::run() {
    createSandboxScene();

    FreeLookCameraController cameraController;

    auto lastFrameTime = static_cast<float>(glfwGetTime());
    while (!window.should_close()) {

        /* currentFrame info */
        float currentFrameTime = static_cast<float>(glfwGetTime());
        float dt = (currentFrameTime - lastFrameTime);
        // dt = std::min(dt, 0.1f); // prevent spark when replay after paused(debugging..)
        lastFrameTime = currentFrameTime;

        /* update */
        window.pollEvents();
        input.update();
        cameraController.update(scene.camera, input, dt);

        const WindowSize& size = window.get_size();
        const MouseState& mouseState = input.getMouseState();

        scene.update(dt);
        renderer.beginFrame(scene, size);
        renderer.render(scene);
        renderer.endFrame();
        editor.draw(scene);

        /* render debug */
        ImGui::SetNextWindowSize(ImVec2(250,150), ImGuiCond_Once);
        ImGui::Begin("Debug");
        ImGui::Text("FPS %.1f FPS", ImGui::GetIO().Framerate);
        ImGui::Text("size : %dx%d", size.w, size.h);
        ImGui::Text("buffer size : %dx%d", size.fb_w, size.fb_h);
        ImGui::Text("screen pos : %.2fx%.2f", mouseState.screenX,mouseState.screenY);
        ImGui::Text("cursor delta : %.2fx%.2f", mouseState.deltaX,mouseState.deltaY);

        ImGui::Text(
            "Left : %s",
            mouseState.leftBtnDown ? "Pressed" : "NONE"
        );

        ImGui::Text(
            "Right : %s",
            mouseState.rightBtnDown ? "Pressed" : "NONE"
        );

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

        if (!scene.directionalLights.empty()) {
            ImGui::SeparatorText("Directional Light");
            ImGui::DragFloat3("directionalLight.direction", glm::value_ptr(scene.directionalLights[0].direction), 0.1f);
            ImGui::DragFloat("directionalLight.intensity", &scene.directionalLights[0].intensity, 0.1f);
        }

        if (!scene.pointLights.empty()) {
            ImGui::SeparatorText("Point Light");
            ImGui::DragFloat("pointLight.intensity", &scene.pointLights[0].intensity, 0.1f);
            ImGui::DragFloat3("pointLight.position", glm::value_ptr(scene.pointLights[0].position), 0.1f);
        }

        if (!scene.spotLights.empty()) {
            ImGui::SeparatorText("Spot Light");
            ImGui::DragFloat("spotLight.intensity", &scene.spotLights[0].intensity, 0.1f);
            ImGui::DragFloat("spotLight.range", &scene.spotLights[0].range, 0.1f);
            ImGui::DragFloat("spotLight.innerAngle", &scene.spotLights[0].innerAngle, 0.1f);
            ImGui::DragFloat("spotLight.outerAngle", &scene.spotLights[0].outerAngle, 0.1f);
            ImGui::DragFloat3("spotLight.position", glm::value_ptr(scene.spotLights[0].position), 0.1f);
            ImGui::DragFloat3("spotLight.color", glm::value_ptr(scene.spotLights[0].color), 0.1f);
        }

        // ImGui::SeparatorText("LitMaterial");
        // ImGui::DragFloat("shininess", &boxMaterial.shininess, 0.1f);
        // ImGui::DragFloat("specularStrength", &boxMaterial.specularStrength, 0.1f);

        ImGui::End();

        window.update();
    }
}

void Application::createSandboxScene() {

    /* camera */
    scene.camera.transform.position.x = 3.0f;
    scene.camera.transform.position.z = 3.0f;
    scene.camera.transform.position.y = 3.0f;
    scene.camera.lookAt(glm::vec3(0.0f));


    /* meshes*/
    const Mesh& planeMesh = resourceManager.getPlaneMesh();

    /* textures */
    const Texture2D& whiteTexture = resourceManager.loadTexture("builtin:white");
    const Texture2D& boxTexture = resourceManager.loadTexture("textures/box.png");
    const Texture2D& boxSpecularMapTexture = resourceManager.loadTexture("textures/box_specular_map.png");
    const Texture2D& grassTexture = resourceManager.loadTexture("textures/grass.png");

    /* shaders */
    const Shader& litShader = resourceManager.getLitShader();
    const Shader& unlitShader = resourceManager.getUnlitShader();

    /* materials */
    LitMaterial& whiteMaterial = resourceManager.loadLitMaterial("white", litShader, whiteTexture);

    LitMaterial& boxMaterial = resourceManager.loadLitMaterial("box", litShader, boxTexture);
    boxMaterial.baseColor = {0.2f,0.2f,0.2f,1.0f};
    boxMaterial.specularTexture = &boxSpecularMapTexture;

    LitMaterial& windowMaterial = resourceManager.loadLitMaterial("window", litShader, whiteTexture);
    windowMaterial.baseColor = {1.0f,0.0f,0.0f,0.3f};

    UnlitMaterial& grassMaterial = resourceManager.loadUnlitMaterial("grass", unlitShader, grassTexture);

    /* models */
    const Model& model = resourceManager.loadModel("models/backpack/backpack.obj");
    const Texture2D& modelTexture = resourceManager.loadTexture("models/backpack/diffuse.jpg");
    const Texture2D& modelSpecularMap = resourceManager.loadTexture("models/backpack/specular.jpg");
    LitMaterial& modelMaterial = resourceManager.loadLitMaterial("model",litShader, modelTexture);
    modelMaterial.specularTexture = &modelSpecularMap;

    Entity& ground = scene.createEntity("ground");
    ground.localTransform.position = {0.0f,0.0f,0.0f};
    ground.localTransform.rotation =  {-90.0f,0.0f,0.0f};
    ground.localTransform.scale = {5.0f,5.0f,5.0f};
    ground.meshRenderer = {&planeMesh, &whiteMaterial};

    Entity& window = scene.createEntity("window");
    window.localTransform.position = {0.0f, 0.0f, 5.0f};
    window.localTransform.rotation = {0.0f, 0.0f, 0.0f};
    window.localTransform.scale = {10.0f, 10.0f, 10.0f};
    window.meshRenderer = {&planeMesh, &windowMaterial};

    Entity& box = scene.createEntity("box");
    box.meshRenderer = {&resourceManager.getCubeMesh(), &whiteMaterial};

    Entity& grass = scene.createEntity("grass");
    grass.localTransform.position = {0.0f, 0.5f, 2.0f};
    grass.localTransform.rotation = {0.0f, 0.0f, 0.0f};
    grass.localTransform.scale = {1.0f, 1.0f, 1.0f};
    grass.meshRenderer = {&planeMesh, &grassMaterial};
    scene.setParent(grass.id, box.id);

    // Entity& bag = scene.createEntity("bag");
    // bag.transform = {
    //     .position = {0.0f,1.0f,0.0f},
    //     .rotation = {0.0f,0.0f,0.0f},
    //     .scale = {0.5f,0.5f,0.5f},
    // };

    // for (auto& mesh : model.meshes) {
    //     scene.entities.push_back({
    //         .transform = ,
    //         .mesh = mesh.get(),
    //         .material = &modelMaterial,
    //     });
    // }

    /* lights */
    scene.ambientLight.intensity = 0.1f;
    scene.pointLights.push_back({
        .position = {0.0f, 1.5f,1.5f},
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
        .range = 5.0f,
        .color = glm::vec3{0.8f,0.4f, 0.4f},
        .intensity = 10.0f,
    });
}
