#include "Game.h"

#include "components/TestComponent.h"
#include "scenes/SandboxScene.h"
#include <scene/ComponentTypeRegistry.h>

void Game::registerComponents(ComponentTypeRegistry &componentTypes) {
    componentTypes.registerComponent<TestComponent>("Test Component");
}

void Game::createInitialScene(
    Scene &scene,
    ResourceManager &resourceManager
) {
    SandboxScene::build(scene, resourceManager);
}
