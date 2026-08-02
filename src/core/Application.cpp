#include "Application.h"

#include <array>

#include "../camera/FreeLookCameraController.h"
#include "../rendering/Lights.h"
#include "../rendering/mesh/MeshRendererComponent.h"
#include "../rendering/skybox/SkyboxComponent.h"

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
        if (Entity *cameraEntity = scene.getActiveCameraEntity()) {
            Transform &cameraTransform =
                    cameraEntity->getComponent<TransformComponent>().local;
            cameraController.update(cameraTransform, input, dt);
        }

        const WindowSize &size = window.get_size();
        const MouseState &mouseState = input.getMouseState();

        editor.beginFrame();
        const RenderExtent viewport = editor.drawSceneView(finalFBO.getTextureId());
        const bool canRenderScene = viewport.width > 0 && viewport.height > 0;

        scene.updateEditor(dt);

        if (canRenderScene) {
            sceneFBO.resize(viewport);
            finalFBO.resize(viewport);

            sceneFBO.bind();
            renderer.beginFrame(scene, viewport);
            renderer.render(scene, {
                .highlightedEntityId = editor.getSelectedEntityId()
            });
            renderer.endFrame();

            /* post process */
            postProcessor.render(sceneFBO, finalFBO);

        }

        FrameBuffer::bindDefault({size.fb_w, size.fb_h});
        glClear(GL_COLOR_BUFFER_BIT);

        editor.draw(scene, size, mouseState);

        window.update();
    }
}

void Application::createSandboxScene() {
    /* camera */
    Entity &cameraEntity = scene.createEntity("Editor Camera");
    Transform &cameraTransform = cameraEntity.getComponent<TransformComponent>().local;
    cameraTransform.position = {3.0f, 3.0f, 3.0f};
    cameraTransform.lookAt(glm::vec3(0.0f));
    cameraEntity.addComponent<CameraComponent>();
    scene.setActiveCamera(cameraEntity.id);


    /* meshes*/
    const Mesh &planeMesh = resourceManager.getPlaneMesh();

    /* textures */
    const Texture2D &whiteTexture = resourceManager.getWhiteTexture();
    const Texture2D &boxTexture = resourceManager.loadTexture("textures/box.png");
    const Texture2D &boxSpecularMapTexture = resourceManager.loadTexture("textures/box_specular_map.png");
    const Texture2D &grassTexture = resourceManager.loadTexture("textures/grass.png");
    const std::array<std::string, 6> skyboxFaces{
        "textures/skybox/right.jpg",
        "textures/skybox/left.jpg",
        "textures/skybox/top.jpg",
        "textures/skybox/bottom.jpg",
        "textures/skybox/front.jpg",
        "textures/skybox/back.jpg"
    };
    const CubeMap &skyboxCubeMap = resourceManager.loadCubeMap("defaultSkybox", skyboxFaces);

    /* shaders */
    const Shader &litShader = resourceManager.getLitShader();
    const Shader &unlitShader = resourceManager.getUnlitShader();

    Entity &skybox = scene.createEntity("Skybox");
    skybox.addComponent<SkyboxComponent>(&skyboxCubeMap);

    /* materials */
    LitMaterial &whiteMaterial = resourceManager.loadLitMaterial("white", litShader, whiteTexture);

    LitMaterial &boxMaterial = resourceManager.loadLitMaterial("box", litShader, boxTexture);
    boxMaterial.baseColor = {0.2f, 0.2f, 0.2f, 1.0f};
    boxMaterial.specularTexture = &boxSpecularMapTexture;

    LitMaterial &windowMaterial = resourceManager.loadLitMaterial("window", litShader, whiteTexture);
    windowMaterial.baseColor = {1.0f, 0.0f, 0.0f, 0.3f};
    windowMaterial.rasterState.cullMode = CullMode::None;

    UnlitMaterial &grassMaterial = resourceManager.loadUnlitMaterial("grass", unlitShader, grassTexture);
    grassMaterial.rasterState.cullMode = CullMode::None;

    /* models */
    const Model &bagModel = resourceManager.loadModel("models/backpack/backpack.obj", true);
    const Model &fourArmsModel = resourceManager.loadModel("models/ben10-four-arms.glb", false);
    const Model &fireManModel = resourceManager.loadModel("models/fire-elementals.glb", false);

    Entity &ground = scene.createEntity("ground");
    Transform &groundTransform = ground.getComponent<TransformComponent>().local;
    groundTransform.position = {0.0f, 0.0f, 0.0f};
    groundTransform.rotation = {-90.0f, 0.0f, 0.0f};
    groundTransform.scale = {5.0f, 5.0f, 5.0f};
    MeshRendererComponent &groundRenderer =
            ground.addComponent<MeshRendererComponent>(&planeMesh, &whiteMaterial);
    groundRenderer.outlineMode = OutlineMode::ScaleFromPivot;

    Entity &box = scene.createEntity("box");
    EntityId boxId = box.id;
    MeshRendererComponent &boxRenderer = box.addComponent<MeshRendererComponent>(
        &resourceManager.getCubeMesh(),
        &whiteMaterial
    );
    boxRenderer.outlineMode = OutlineMode::ScaleFromPivot;

    Entity &box2 = scene.createEntity("box2");
    MeshRendererComponent &box2Renderer = box2.addComponent<MeshRendererComponent>(
        &resourceManager.getCubeMesh(),
        &whiteMaterial
    );
    box2Renderer.outlineMode = OutlineMode::ScaleFromPivot;
    box2.getComponent<TransformComponent>().local.position.x = 3.0f;
    box2.siblingIndex = 0;
    scene.moveEntity(box2.id, boxId, 1);

    auto bagId = scene.instantiateModel(bagModel, whiteMaterial, "bag");
    Entity *bagEntity = scene.findEntity(bagId);
    bagEntity->getComponent<TransformComponent>().local = {
        .position = {0.0f, 1.0f, 0.0f},
        .rotation = {0.0f, 0.0f, 0.0f},
        .scale = {0.5f, 0.5f, 0.5f},
    };

    auto fourArmsId = scene.instantiateModel(fourArmsModel, whiteMaterial, "fourArms");
    Entity *fourArmsEntity = scene.findEntity(fourArmsId);
    fourArmsEntity->getComponent<TransformComponent>().local = {
        .position = {-2.0f, 1.0f, 0.0f},
        .rotation = {0.0f, 0.0f, 0.0f},
        .scale = {1.0f, 1.0f, 1.0f},
    };

    auto fireManId = scene.instantiateModel(fireManModel, whiteMaterial, "fireman");
    Entity *fireManEntity = scene.findEntity(fireManId);
    fireManEntity->getComponent<TransformComponent>().local = {
        .position = {1.0f, 1.0f, 1.0f},
        .rotation = {0.0f, 0.0f, 0.0f},
        .scale = {1.0f, 1.0f, 1.0f},
    };

    // TODO: transparent entities must be re-ordered in scene render pipeline.
    // must be render after none-transparent object.
    Entity &grass = scene.createEntity("grass");
    Transform &grassTransform = grass.getComponent<TransformComponent>().local;
    grassTransform.position = {0.0f, 0.5f, 2.0f};
    grassTransform.rotation = {0.0f, 0.0f, 0.0f};
    grassTransform.scale = {1.0f, 1.0f, 1.0f};
    MeshRendererComponent &grassRenderer =
            grass.addComponent<MeshRendererComponent>(&planeMesh, &grassMaterial);
    grassRenderer.outlineMode = OutlineMode::ScaleFromPivot;
    grass.siblingIndex = 3;

    Entity &window = scene.createEntity("window");
    Transform &windowTransform = window.getComponent<TransformComponent>().local;
    windowTransform.position = {0.0f, 0.0f, 5.0f};
    windowTransform.rotation = {0.0f, 0.0f, 0.0f};
    windowTransform.scale = {10.0f, 10.0f, 10.0f};
    MeshRendererComponent &windowRenderer =
            window.addComponent<MeshRendererComponent>(&planeMesh, &windowMaterial);
    windowRenderer.outlineMode = OutlineMode::ScaleFromPivot;

    /* lights */
    Entity &ambientLightEntity = scene.createEntity("Ambient Light");
    AmbientLightComponent &ambientLight =
            ambientLightEntity.addComponent<AmbientLightComponent>();
    ambientLight.intensity = 0.1f;

    Entity &pointLightEntity = scene.createEntity("Point Light");
    pointLightEntity.getComponent<TransformComponent>().local.position = {0.0f, 1.5f, 1.5f};
    PointLightComponent &pointLight = pointLightEntity.addComponent<PointLightComponent>();
    pointLight.range = 5.0f;
    pointLight.color = {1.0f, 1.0f, 1.0f};
    pointLight.intensity = 1.0f;

    Entity &directionalLightEntity = scene.createEntity("Directional Light");
    directionalLightEntity.getComponent<TransformComponent>().local.rotation = {-90.0f, 0.0f, 0.0f};
    DirectionalLightComponent &directionalLight =
            directionalLightEntity.addComponent<DirectionalLightComponent>();
    directionalLight.color = {1.0f, 1.0f, 1.0f};
    directionalLight.intensity = 0.1f;

    Entity &spotLightEntity = scene.createEntity("Spot Light");
    Transform &spotLightTransform = spotLightEntity.getComponent<TransformComponent>().local;
    spotLightTransform.position = {0.0f, 4.0f, 0.0f};
    spotLightTransform.rotation = {-90.0f, 0.0f, 0.0f};
    SpotLightComponent &spotLight = spotLightEntity.addComponent<SpotLightComponent>();
    spotLight.range = 5.0f;
    spotLight.color = {0.8f, 0.4f, 0.4f};
    spotLight.intensity = 10.0f;
}
