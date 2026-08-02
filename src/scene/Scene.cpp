#include "../scene/Scene.h"

#include <algorithm>
#include <cmath>
#include <format>

#include <glm/gtc/matrix_inverse.hpp>

Scene::~Scene() {
    // Detach components while the Scene and its services are still alive.
    entities.clear();
}

bool Scene::wouldCreateCycle(EntityId childId, EntityId parentId) {
    Entity *current = findEntity(parentId);
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
    const size_t siblingIndex = getChildren(std::nullopt).size();

    return entities.emplace_back(*this, entitySeq++, name, siblingIndex);
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

const Entity *Scene::findEntity(EntityId id) const {
    auto it = std::find_if(entities.begin(), entities.end(), [id](const Entity &entity) {
        return entity.id == id;
    });

    if (it == entities.end()) {
        return nullptr;
    }

    return &(*it);
}

std::vector<const Entity *> Scene::getChildren(std::optional<EntityId> id) const {
    std::vector<const Entity *> children;
    for (const Entity &entity: entities) {
        if (entity.parentId == id) {
            children.push_back(&entity);
        }
    }

    std::ranges::sort(children, {}, [](const Entity *entity) {
        return entity->siblingIndex;
    });

    return children;
}

std::vector<EntityId> Scene::getChildrenIds(std::optional<EntityId> id, std::optional<EntityId> excludeId) const {
    std::vector<EntityId> result;
    const auto children = getChildren(id);
    for (const Entity *entity: children) {
        if (excludeId && entity->id == *excludeId) {
            continue;
        }
        result.push_back(entity->id);
    }
    return result;
}

bool Scene::moveEntity(EntityId sourceId, std::optional<EntityId> newParentId, size_t insertIndex,
                       bool keepLocalTransform) {
    Entity *source = findEntity(sourceId);
    if (!source) {
        return false;
    }

    // parent must be existed if assigned
    Entity *newParent = newParentId ? findEntity(*newParentId) : nullptr;
    if (newParentId && !newParent) {
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

    const std::optional<EntityId> oldParentId = source->parentId;
    const bool parentChanged = oldParentId != newParentId;
    const size_t oldSiblingIndex = source->siblingIndex;

    auto newSiblingIds = getChildrenIds(newParentId, sourceId);

    // insertIndex is based on the list before removing source.
    // Moving downward in the same list shifts the target slot left by one.
    if (!parentChanged && oldSiblingIndex < insertIndex) {
        --insertIndex;
    }

    Transform newLocalTransform = source->getComponent<TransformComponent>().local;

    if (!keepLocalTransform && parentChanged) {
        // Keep the entity at the same world transform after reparenting.
        const glm::mat4 oldWorldMatrix = getWorldMatrix(*source);
        const glm::mat4 newParentWorldMatrix =
                newParent ? getWorldMatrix(*newParent) : glm::mat4{1.0f};
        const float determinant = glm::determinant(newParentWorldMatrix);

        if (!std::isfinite(determinant) || std::abs(determinant) <= 1e-6f) {
            LOG(std::format("Cannot move entity {}: parent transform is not invertible", sourceId));
            return false;
        }

        const glm::mat4 newLocalMatrix =
                glm::inverse(newParentWorldMatrix) * oldWorldMatrix;

        if (!Transform::decompose(newLocalMatrix, newLocalTransform)) {
            LOG(std::format("Failed to decompose transform {}", sourceId));
            return false;
        }
    }

    // Apply hierarchy changes only after every validation has succeeded.
    if (parentChanged) {
        auto oldSiblingIds = getChildrenIds(oldParentId, sourceId);
        for (size_t i = 0; i < oldSiblingIds.size(); ++i) {
            findEntity(oldSiblingIds[i])->siblingIndex = i;
        }
    }

    source->getComponent<TransformComponent>().local = newLocalTransform;
    source->parentId = newParentId;

    insertIndex = std::min(insertIndex, newSiblingIds.size());
    newSiblingIds.insert(
        newSiblingIds.begin() +
        static_cast<std::vector<EntityId>::difference_type>(insertIndex),
        sourceId
    );

    for (size_t i = 0; i < newSiblingIds.size(); ++i) {
        findEntity(newSiblingIds[i])->siblingIndex = i;
    }

    return true;
}

glm::mat4 Scene::getWorldMatrix(const Entity &entity) const {
    const Transform &localTransform = entity.getComponent<TransformComponent>().local;
    glm::mat4 localMatrix = localTransform.getLocalMatrix();
    if (!entity.parentId) {
        return localMatrix;
    }

    const Entity *parentEntity = findEntity(*entity.parentId);
    if (!parentEntity) {
        return localMatrix;
    }

    return getWorldMatrix(*parentEntity) * localMatrix;
}

EntityId Scene::instantiateModel(const Model &model, const Material &fallbackMaterial, const std::string &name) {
    const EntityId rootEntityId = createEntity(name).id;

    const auto &nodes = model.nodes;
    const auto &parts = model.parts;

    std::vector<EntityId> nodeEntityIds(nodes.size());

    for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++) {
        const ModelNode &modelNode = nodes[nodeIndex];

        Entity &nodeEntity = createEntity(modelNode.name);
        const EntityId nodeEntityId = nodeEntity.id;

        nodeEntityIds[nodeIndex] = nodeEntityId;
        nodeEntity.getComponent<TransformComponent>().local = modelNode.localTransform;
        std::optional<EntityId> parentId = modelNode.parentIndex
                                               ? std::optional<EntityId>(nodeEntityIds[*modelNode.parentIndex])
                                               : std::optional<EntityId>(rootEntityId);
        size_t insertIndex = getChildren(parentId).size();
        moveEntity(nodeEntityId, *parentId, insertIndex, true);

        const auto resolvePartMaterial =
                [&](const ModelPart &part)
            -> const Material * {
            if (part.materialSlot <
                model.materials.size()) {
                const Material *material =
                        model.materials[
                            part.materialSlot
                        ];

                if (material) {
                    return material;
                }
            }

            return &fallbackMaterial;
        };

        if (modelNode.partIndices.size() == 1) {
            const ModelPart &part = parts[modelNode.partIndices[0]];
            Entity *target = findEntity(nodeEntityId);
            target->meshRenderComponent = {
                .mesh = part.mesh.get(),
                .material = resolvePartMaterial(part)
            };
        } else {
            // one node has multiple meshes
            for (size_t partOrder = 0; partOrder < modelNode.partIndices.size(); partOrder++) {
                auto partIndex = modelNode.partIndices[partOrder];
                const ModelPart &part = parts[partIndex];
                Entity &meshEntity = createEntity(std::format("{}_Mesh_{}", modelNode.name, partOrder));
                const EntityId meshEntityId = meshEntity.id;

                meshEntity.meshRenderComponent = {
                    .mesh = part.mesh.get(),
                    .material = resolvePartMaterial(part)
                };

                moveEntity(meshEntityId, nodeEntityId, partOrder, true);
            }
        }
    }

    return rootEntityId;
}
