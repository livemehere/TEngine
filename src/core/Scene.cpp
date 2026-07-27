#include "Scene.h"

void Scene::update(float dt) {
    camera.update(dt);
}

Entity & Scene::createEntity(const std::string &name) {
    Entity& entity = entities.emplace_back();
    entity.id = nextEntityId++;
    entity.name = name;

    return entity;
}


