#include "TestComponent.h"

#include "../../scene/Entity.h"
#include "../../scene/Scene.h"
#include "../../scene/TransformComponent.h"

void TestComponent::update(float dt) {
    Entity *entity = getScene().findEntity(getEntityId());
    if (!entity) {
        return;
    }

    Transform &transform =
            entity->getComponent<TransformComponent>().local;
    transform.position.x += speed * dt;
}
