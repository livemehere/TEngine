#include "Scene.h"

void Scene::update(float dt) {
    camera.update(dt);
}

Entity &Scene::createEntity(const std::string &name) {
    Entity &entity = entities.emplace_back();
    entity.id = entitySeq++;
    entity.name = name;
    return entity;
}

Entity *Scene::findEntity(const EntityId id) {
    auto it = std::find_if(entities.begin(), entities.end(), [id](const Entity &entity) {
        return entity.id == id;
    });

    if (it == entities.end()) {
        return nullptr;
    }

    return &(*it);
}
