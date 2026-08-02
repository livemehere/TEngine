#pragma once

#include <core/GameModule.h>

class Game final : public GameModule {
public:
    void registerComponents(
        ComponentTypeRegistry &componentTypes
    ) override;

    void createInitialScene(
        Scene &scene,
        ResourceManager &resourceManager
    ) override;
};
