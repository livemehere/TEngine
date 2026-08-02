#pragma once

#include "Input.h"
#include "utils.h"
#include "Window.h"
#include "../rendering/Renderer.h"
#include "../resources/ResourceManager.h"
#include "../scene/Scene.h"
#include "../editor/Editor.h"
#include "../graphics/FrameBuffer.h"
#include "../rendering/PostProcessor.h"

class Application {
    Window window;
    Input input;

    ResourceManager resourceManager;

    Renderer renderer;
    PostProcessor postProcessor;
    FrameBuffer sceneFBO;
    FrameBuffer finalFBO;

    Scene scene;
    Editor editor;
public:
    Application()
        : window(1920, 1080, "TEngine", true),
          input(window),
          resourceManager(ASSET_ROOT),
          renderer(resourceManager),
          postProcessor(resourceManager),
          sceneFBO({.extent = {1, 1}, .hasDepthStencil = true}),
          finalFBO({.extent = {1, 1}, .hasDepthStencil = false}) {}
    ~Application() = default;

    void run();
    void createSandboxScene();
};
