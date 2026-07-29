#include "Scene.h"

#include <algorithm>

bool Scene::wouldCreateCycle(EntityId childId, EntityId parentId) {
    Entity* current =  findEntity(parentId);
    while (current) {

        if (current->id == childId) {
            // make cycle, disable set parent
            return true;
        }

        if (!current->parentId) {
            break;
        }
        current = findEntity(*current->parentId);
    }

    // enable to set parent
    return false;
}

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

const Entity * Scene::findEntity(EntityId id) const {
    auto it = std::find_if(entities.begin(), entities.end(), [id](const Entity &entity) {
    return entity.id == id;
});

    if (it == entities.end()) {
        return nullptr;
    }

    return &(*it);
}

std::vector<const Entity *> Scene::getChildren(std::optional<EntityId> id) const {
    std::vector<const Entity*> children;
    for (const Entity& entity : entities) {
       if (entity.parentId == id) {
           children.push_back(&entity);
       }
    }

    std::ranges::sort(children, {}, [](const Entity* entity) {
        return entity->siblingIndex;
    });

    return children;
}

bool Scene::setParent(EntityId childId, EntityId parentId) {
    if (childId == parentId) return false;

    Entity* child = findEntity(childId);
    if (!child) {
        return false;
    }

    // check cycle
    if (wouldCreateCycle(childId, parentId)) {
        return false;
    }

    // keep child world position
    // TODO:

    child->parentId = parentId;

    return true;
}

bool Scene::unsetParent(EntityId childId, EntityId parentId) {
}

glm::mat4 Scene::getWorldMatrix(const Entity &entity) const {
    glm::mat4 localMatrix = entity.localTransform.getLocalMatrix();
    if (!entity.parentId) {
        return localMatrix;
    }

    const Entity* parentEntity = findEntity(*entity.parentId);
    if (!parentEntity) {
        return localMatrix;
    }

    return getWorldMatrix(*parentEntity) * localMatrix;
}
