#include "Application.h"

#include "GameModule.h"
#include "utils.h"
#include "../camera/FreeLookCameraController.h"
#include "../scene/SceneSerializer.h"

Application::Application(GameModule &game)
    : game(game),
      window(1920, 1080, "TEngine", true),
      input(window),
      resourceManager(ASSET_ROOT),
      renderer(resourceManager),
      postProcessor(resourceManager),
      sceneFBO({.extent = {1, 1}, .hasDepthStencil = true}),
      finalFBO({.extent = {1, 1}, .hasDepthStencil = false}),
      editor(componentTypes, resourceManager) {
    registerBuiltinComponentTypes(componentTypes);
    game.registerComponents(componentTypes);
}

void Application::run() {
    game.createInitialScene(scene, resourceManager);

    FreeLookCameraController cameraController;

    auto lastFrameTime = static_cast<float>(glfwGetTime());
    while (!window.should_close()) {
        const float currentFrameTime = static_cast<float>(glfwGetTime());
        const float dt = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        window.pollEvents();
        input.update();

        const WindowSize &size = window.get_size();
        const MouseState &mouseState = input.getMouseState();

        Scene &displayedScene = runtimeScene ? *runtimeScene : scene;

        editor.beginFrame();
        const RenderExtent viewport = editor.drawSceneView(
            displayedScene,
            finalFBO.getTextureId(),
            runtimeScene != nullptr
        );
        const bool canRenderScene = viewport.width > 0 && viewport.height > 0;

        if (const std::optional<PlayModeRequest> request =
                editor.consumePlayModeRequest()) {
            if (*request == PlayModeRequest::Play && !runtimeScene) {
                runtimeScene = SceneSerializer::cloneForRuntime(
                    scene,
                    componentTypes
                );
            } else if (*request == PlayModeRequest::Stop && runtimeScene) {
                runtimeScene->stopRuntime();
                runtimeScene.reset();
            }
        }

        Scene &activeScene = runtimeScene ? *runtimeScene : scene;

        if (Entity *cameraEntity = activeScene.getActiveCameraEntity()) {
            Transform &cameraTransform =
                    cameraEntity->getComponent<TransformComponent>().local;
            cameraController.update(cameraTransform, input, dt);
        }

        if (runtimeScene) {
            activeScene.updateRuntime(dt);
        } else {
            activeScene.updateEditor(dt);
        }

        if (canRenderScene) {
            sceneFBO.resize(viewport);
            finalFBO.resize(viewport);

            sceneFBO.bind();
            renderer.beginFrame(activeScene, viewport, renderSettings);
            renderer.render(activeScene, {
                .highlightedEntityId = editor.getSelectedEntityId()
            });
            renderer.endFrame();

            postProcessor.render(sceneFBO, finalFBO);
        }

        FrameBuffer::bindDefault({size.fb_w, size.fb_h});
        glClear(GL_COLOR_BUFFER_BIT);

        editor.draw(
            activeScene,
            size,
            mouseState,
            renderSettings,
            renderer.getStats()
        );
        window.update();
    }

    if (runtimeScene) {
        runtimeScene->stopRuntime();
        runtimeScene.reset();
    }
}
