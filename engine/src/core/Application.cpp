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
      bloomProcessor(resourceManager),
      postProcessor(resourceManager),
      sceneFBO({
          .extent = {1, 1},
          .hasDepthStencil = true,
          .colorFormat = FrameBufferColorFormat::RGBA16F
      }),
      resolvedSceneFBO({
          .extent = {1, 1},
          .hasDepthStencil = false,
          .colorFormat = FrameBufferColorFormat::RGBA16F
      }),
      finalFBO({
          .extent = {1, 1},
          .hasDepthStencil = false,
          .colorFormat = FrameBufferColorFormat::RGBA8
      }),
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
            renderer.getFrameBufferDebugTextureId(),
            renderSettings,
            runtimeScene != nullptr,
            mouseState
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
            const int sceneSamples =
                    renderSettings.renderingPath == RenderingPath::Deferred
                        ? 1
                        : renderSettings.msaaSamples;
            sceneFBO.setSamples(sceneSamples);
            sceneFBO.resize(viewport);
            resolvedSceneFBO.resize(viewport);
            finalFBO.resize(viewport);

            sceneFBO.bind();
            renderer.beginFrame(activeScene, viewport, renderSettings);
            renderer.render(activeScene, {
                .highlightedEntityId = editor.getSelectedEntityId()
            });
            renderer.endFrame();

            const FrameBuffer *postProcessSource = &sceneFBO;
            if (sceneFBO.isMultisampled()) {
                sceneFBO.resolveTo(resolvedSceneFBO);
                postProcessSource = &resolvedSceneFBO;
            }

            const FrameBuffer* bloom = bloomProcessor.process(
                *postProcessSource,
                renderSettings
            );

            postProcessor.render(
                *postProcessSource,
                bloom,
                finalFBO,
                renderSettings
            );
            finalFBO.bind();
            renderer.renderCanvas(activeScene, viewport);
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
