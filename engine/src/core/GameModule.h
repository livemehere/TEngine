#pragma once

class ComponentTypeRegistry;
class ResourceManager;
class Scene;

class GameModule {
public:
    virtual ~GameModule() = default;

    virtual void registerComponents(
        ComponentTypeRegistry &componentTypes
    ) {}

    virtual void createInitialScene(
        Scene &scene,
        ResourceManager &resourceManager
    ) = 0;
};
