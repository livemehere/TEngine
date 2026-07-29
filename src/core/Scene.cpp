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

std::vector<EntityId> Scene::getChildrenIds(std::optional<EntityId> id, std::optional<EntityId> excludeId) const {
    std::vector<EntityId> result;
    const auto children = getChildren(id);
    for (const Entity* entity : children) {
        if (excludeId && entity->id == *excludeId) {
            continue;
        }
        result.push_back(entity->id);
    }
    return result;
}

bool Scene::moveEntity(EntityId sourceId, std::optional<EntityId> newParentId, size_t insertIndex) {
    Entity* source = findEntity(sourceId);
    if (!source) {
        return false;
    }

    // parent must be existed if assigned
    if (newParentId && !findEntity(*newParentId)) {
        return false;
    }

    // disable self
    if (newParentId && sourceId == *newParentId) {
        return false;
    }

    // check cycle
    if (newParentId && wouldCreateCycle(sourceId, *newParentId)) {
        return false;
    }

    std::optional<EntityId> oldParentId = source->parentId;

    auto newSiblingIds = getChildrenIds(newParentId, sourceId);

    if (oldParentId != newParentId) {
       auto oldSiblings = getChildrenIds(oldParentId, sourceId);
        for (size_t i=0; i< oldSiblings.size(); i++) {
            findEntity(oldSiblings[i])->siblingIndex = i;
        }
    }

    // keep child world position
    // TODO:
    const glm::mat4 oldWorldMatrix = getWorldMatrix(*source);
    const glm::mat4 newParentWorldMatrix = newParentId ? getWorldMatrix(*findEntity(*newParentId)) : glm::mat4{1.0f};
    const glm::mat4 newLocalMatrix = glm::inverse(newParentWorldMatrix) * oldWorldMatrix;

    if (!Transform::decompose(newLocalMatrix, source->localTransform)) {
        LOG(std::format("Failed to decompose transform {}", sourceId));
    }

    source->parentId = newParentId;
    insertIndex = std::min(insertIndex, newSiblingIds.size());
    newSiblingIds.insert(newSiblingIds.begin() + insertIndex, sourceId);
    for (size_t i=0; i<newSiblingIds.size(); i++) {
        findEntity(newSiblingIds[i])->siblingIndex = i;
    }
    return true;
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
