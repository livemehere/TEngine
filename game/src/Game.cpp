#include "Game.h"

#include "components/MotionComponents.h"
#include "components/TestComponent.h"
#include "scenes/SandboxScene.h"
#include <scene/ComponentTypeRegistry.h>

void Game::registerComponents(ComponentTypeRegistry &componentTypes) {
    componentTypes.registerComponent<TestComponent>("Test Component");
    componentTypes.registerComponent<SpinComponent>("Spin");
    componentTypes.registerComponent<BobComponent>("Bob");
    componentTypes.registerComponent<OrbitComponent>("Orbit");
}

void Game::createInitialScene(
    Scene &scene,
    ResourceManager &resourceManager
) {
    SandboxScene::build(scene, resourceManager);
}
