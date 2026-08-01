#pragma once

#include "Input.h"
#include "utils.h"
#include "Window.h"
#include "../rendering/Renderer.h"
#include "../resources/ResourceManager.h"
#include "../scene/Scene.h"
#include "../editor/Editor.h"

class Application {
    Window window;
    Input input;

    ResourceManager resourceManager;

    Renderer renderer;

    Scene scene;
    Editor editor;
public:
    Application()
        : window(1920, 1080, "TEngine", true),
          input(window),
          resourceManager(ASSET_ROOT),
          renderer(resourceManager) {}
    ~Application() = default;

    void run();
    void createSandboxScene();
};
