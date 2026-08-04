#pragma once

#include <memory>

#include "Input.h"
#include "Window.h"
#include "../rendering/Renderer.h"
#include "../resources/ResourceManager.h"
#include "../scene/Scene.h"
#include "../scene/ComponentTypeRegistry.h"
#include "../editor/Editor.h"
#include "../graphics/FrameBuffer.h"
#include "../rendering/PostProcessor.h"

class GameModule;

class Application {
    GameModule &game;
    Window window;
    Input input;

    ResourceManager resourceManager;

    RenderSettings renderSettings;
    Renderer renderer;
    PostProcessor postProcessor;
    FrameBuffer sceneFBO;
    FrameBuffer resolvedSceneFBO;
    FrameBuffer finalFBO;

    ComponentTypeRegistry componentTypes;
    Scene scene;
    std::unique_ptr<Scene> runtimeScene;
    Editor editor;
public:
    explicit Application(GameModule &game);
    ~Application() = default;

    void run();
};
