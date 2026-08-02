#pragma once

#include <memory>

#include "Input.h"
#include "utils.h"
#include "Window.h"
#include "../rendering/Renderer.h"
#include "../resources/ResourceManager.h"
#include "../scene/Scene.h"
#include "../scene/ComponentTypeRegistry.h"
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

    ComponentTypeRegistry componentTypes;
    Scene scene;
    std::unique_ptr<Scene> runtimeScene;
    Editor editor;
public:
    Application();
    ~Application() = default;

    void run();
    void createSandboxScene();
};
