#pragma once

#include "Input.h"
#include "utils.h"
#include "Window.h"
#include "../rendering/Renderer.h"
#include "../resources/ResourceManager.h"
#include "../Scene.h"

class Application {
    Window window;
    Input input;

    ResourceManager resourceManager;

    Renderer renderer;

    Scene scene;
    // Editor
public:
    Application() : window(1920, 1080, "TEngine", true), input(window), resourceManager(ASSET_ROOT) {};
    ~Application() = default;

    void run();
    void createSandboxScene();
};
