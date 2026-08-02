#include "Application.h"

#include <array>

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

        const WindowSize &size = window.get_size();
        const MouseState &mouseState = input.getMouseState();

        editor.beginFrame();
        const RenderExtent viewport = editor.drawSceneView(finalFBO.getTextureId());
        const bool canRenderScene = viewport.width > 0 && viewport.height > 0;

        scene.update(dt);

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
    scene.camera.transform.position.x = 3.0f;
    scene.camera.transform.position.z = 3.0f;
    scene.camera.transform.position.y = 3.0f;
    scene.camera.lookAt(glm::vec3(0.0f));


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
    skybox.skyboxComponent = {
        .cubeMap = &skyboxCubeMap
    };

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
    ground.localTransform.position = {0.0f, 0.0f, 0.0f};
    ground.localTransform.rotation = {-90.0f, 0.0f, 0.0f};
    ground.localTransform.scale = {5.0f, 5.0f, 5.0f};
    ground.meshRenderComponent = {
        .mesh = &planeMesh,
        .material = &whiteMaterial,
        .outlineMode = OutlineMode::ScaleFromPivot
    };

    Entity &box = scene.createEntity("box");
    EntityId boxId = box.id;
    box.meshRenderComponent = {
        .mesh = &resourceManager.getCubeMesh(),
        .material = &whiteMaterial,
        .outlineMode = OutlineMode::ScaleFromPivot
    };

    Entity &box2 = scene.createEntity("box2");
    box2.meshRenderComponent = {
        .mesh = &resourceManager.getCubeMesh(),
        .material = &whiteMaterial,
        .outlineMode = OutlineMode::ScaleFromPivot
    };
    box2.localTransform.position.x = 3.0f;
    box2.siblingIndex = 0;
    scene.moveEntity(box2.id, boxId, 1);

    auto bagId = scene.instantiateModel(bagModel, whiteMaterial, "bag");
    Entity *bagEntity = scene.findEntity(bagId);
    bagEntity->localTransform = {
        .position = {0.0f, 1.0f, 0.0f},
        .rotation = {0.0f, 0.0f, 0.0f},
        .scale = {0.5f, 0.5f, 0.5f},
    };

    auto fourArmsId = scene.instantiateModel(fourArmsModel, whiteMaterial, "fourArms");
    Entity *fourArmsEntity = scene.findEntity(fourArmsId);
    fourArmsEntity->localTransform = {
        .position = {-2.0f, 1.0f, 0.0f},
        .rotation = {0.0f, 0.0f, 0.0f},
        .scale = {1.0f, 1.0f, 1.0f},
    };

    auto fireManId = scene.instantiateModel(fireManModel, whiteMaterial, "fireman");
    Entity *fireManEntity = scene.findEntity(fireManId);
    fireManEntity->localTransform = {
        .position = {1.0f, 1.0f, 1.0f},
        .rotation = {0.0f, 0.0f, 0.0f},
        .scale = {1.0f, 1.0f, 1.0f},
    };

    // TODO: transparent entities must be re-ordered in scene render pipeline.
    // must be render after none-transparent object.
    Entity &grass = scene.createEntity("grass");
    grass.localTransform.position = {0.0f, 0.5f, 2.0f};
    grass.localTransform.rotation = {0.0f, 0.0f, 0.0f};
    grass.localTransform.scale = {1.0f, 1.0f, 1.0f};
    grass.meshRenderComponent = {
        .mesh = &planeMesh,
        .material = &grassMaterial,
        .outlineMode = OutlineMode::ScaleFromPivot
    };
    grass.siblingIndex = 3;

    Entity &window = scene.createEntity("window");
    window.localTransform.position = {0.0f, 0.0f, 5.0f};
    window.localTransform.rotation = {0.0f, 0.0f, 0.0f};
    window.localTransform.scale = {10.0f, 10.0f, 10.0f};
    window.meshRenderComponent = {
        .mesh = &planeMesh,
        .material = &windowMaterial,
        .outlineMode = OutlineMode::ScaleFromPivot
    };

    /* lights */
    scene.ambientLight.intensity = 0.1f;
    scene.pointLights.push_back({
        .position = {0.0f, 1.5f, 1.5f},
        .range = 5.0f,
        .color = {1.0f, 1.0f, 1.0f},
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
        .color = glm::vec3{0.8f, 0.4f, 0.4f},
        .intensity = 10.0f,
    });
}
