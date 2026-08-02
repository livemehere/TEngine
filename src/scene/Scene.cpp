#include "../scene/Scene.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <stdexcept>

#include <glm/gtc/matrix_inverse.hpp>

#include "../common.h"
#include "../rendering/mesh/MeshRendererComponent.h"
#include "Behaviour.h"

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

void Scene::endComponentIteration() noexcept {
    if (componentIterationDepth == 0) {
        return;
    }

    --componentIterationDepth;
    if (componentIterationDepth == 0) {
        flushPendingComponentChanges();
    }
}

void Scene::flushPendingComponentChanges() noexcept {
    const size_t entityCount = entities.size();
    for (size_t index = 0; index < entityCount; ++index) {
        entities[index].flushPendingComponentChanges();
    }
}

void Scene::updateEditor(float) {
}

void Scene::updateRuntime(float dt) {
    // Start runs once, immediately before the first update of an enabled Behaviour.
    {
        ComponentIterationScope iterationScope(*this);
        const size_t entityCount = entities.size();
        for (size_t index = 0; index < entityCount; ++index) {
            Entity &entity = entities[index];
            for (auto &[type, component]: entity.components_) {
                if (entity.pendingComponentRemovals_.contains(type)) {
                    continue;
                }

                auto *behaviour = dynamic_cast<Behaviour *>(component.get());
                if (!behaviour || !behaviour->enabled || behaviour->started_) {
                    continue;
                }

                behaviour->started_ = true;
                behaviour->start();
            }
        }
    }

    {
        ComponentIterationScope iterationScope(*this);
        const size_t entityCount = entities.size();
        for (size_t index = 0; index < entityCount; ++index) {
            Entity &entity = entities[index];
            for (auto &[type, component]: entity.components_) {
                if (entity.pendingComponentRemovals_.contains(type)) {
                    continue;
                }

                auto *behaviour = dynamic_cast<Behaviour *>(component.get());
                if (behaviour && behaviour->enabled && behaviour->started_) {
                    behaviour->update(dt);
                }
            }
        }
    }

    {
        ComponentIterationScope iterationScope(*this);
        const size_t entityCount = entities.size();
        for (size_t index = 0; index < entityCount; ++index) {
            Entity &entity = entities[index];
            for (auto &[type, component]: entity.components_) {
                if (entity.pendingComponentRemovals_.contains(type)) {
                    continue;
                }

                auto *behaviour = dynamic_cast<Behaviour *>(component.get());
                if (behaviour && behaviour->enabled && behaviour->started_) {
                    behaviour->lateUpdate(dt);
                }
            }
        }
    }
}

void Scene::stopRuntime() {
    ComponentIterationScope iterationScope(*this);
    const size_t entityCount = entities.size();

    for (size_t index = 0; index < entityCount; ++index) {
        Entity &entity = entities[index];
        for (auto &[type, component]: entity.components_) {
            if (entity.pendingComponentRemovals_.contains(type)) {
                continue;
            }

            auto *behaviour = dynamic_cast<Behaviour *>(component.get());
            if (!behaviour || !behaviour->started_) {
                continue;
            }

            behaviour->stop();
            behaviour->started_ = false;
        }
    }
}

std::unique_ptr<Scene> Scene::clone() const {
    auto result = std::make_unique<Scene>();
    result->entitySeq = entitySeq;
    result->activeCameraId = activeCameraId;
    std::vector<Component *> componentsToAttach;

    for (const Entity &source: entities) {
        Entity &target = result->entities.emplace_back(
            *result,
            result->componentIterationDepth,
            source.id,
            source.name,
            source.siblingIndex
        );
        target.parentId = source.parentId;

        const TransformComponent &sourceTransform =
                source.getComponent<TransformComponent>();
        TransformComponent &targetTransform =
                target.getComponent<TransformComponent>();
        targetTransform.enabled = sourceTransform.enabled;
        targetTransform.local = sourceTransform.local;

        for (const auto &[type, component]: source.components_) {
            if (type == std::type_index(typeid(TransformComponent))) {
                continue;
            }

            std::unique_ptr<Component> componentCopy = component->clone();
            if (!componentCopy) {
                throw std::logic_error("Component clone returned null");
            }

            Component *attachedComponent = componentCopy.get();
            attachedComponent->attachTo(*result, target.id);
            target.components_.emplace(type, std::move(componentCopy));
            componentsToAttach.push_back(attachedComponent);
        }
    }

    // Components see a complete hierarchy when their runtime copy is attached.
    for (Component *component: componentsToAttach) {
        component->onAttach();
    }

    return result;
}

Entity &Scene::createEntity(const std::string &name) {
    const size_t siblingIndex = getChildren(std::nullopt).size();

    return entities.emplace_back(
        *this,
        componentIterationDepth,
        entitySeq++,
        name,
        siblingIndex
    );
}

bool Scene::setActiveCamera(EntityId entityId) {
    Entity *entity = findEntity(entityId);
    const std::type_index cameraType = typeid(CameraComponent);
    if (!entity ||
        (!entity->hasComponent<CameraComponent>() &&
         !entity->pendingComponents_.contains(cameraType))) {
        return false;
    }

    activeCameraId = entityId;
    return true;
}

std::optional<EntityId> Scene::getActiveCameraId() const {
    if (!activeCameraId) {
        return std::nullopt;
    }

    const Entity *entity = findEntity(*activeCameraId);
    if (!entity || !entity->hasComponent<CameraComponent>()) {
        return std::nullopt;
    }

    return activeCameraId;
}

Entity *Scene::getActiveCameraEntity() {
    const std::optional<EntityId> cameraId = getActiveCameraId();
    if (!cameraId) {
        return nullptr;
    }

    Entity *entity = findEntity(*cameraId);
    CameraComponent *camera = entity
                                  ? entity->tryGetComponent<CameraComponent>()
                                  : nullptr;
    return camera && camera->enabled ? entity : nullptr;
}

const Entity *Scene::getActiveCameraEntity() const {
    const std::optional<EntityId> cameraId = getActiveCameraId();
    if (!cameraId) {
        return nullptr;
    }

    const Entity *entity = findEntity(*cameraId);
    const CameraComponent *camera = entity
                                        ? entity->tryGetComponent<CameraComponent>()
                                        : nullptr;
    return camera && camera->enabled ? entity : nullptr;
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
            target->addComponent<MeshRendererComponent>(
                part.mesh.get(),
                resolvePartMaterial(part)
            );
        } else {
            // one node has multiple meshes
            for (size_t partOrder = 0; partOrder < modelNode.partIndices.size(); partOrder++) {
                auto partIndex = modelNode.partIndices[partOrder];
                const ModelPart &part = parts[partIndex];
                Entity &meshEntity = createEntity(std::format("{}_Mesh_{}", modelNode.name, partOrder));
                const EntityId meshEntityId = meshEntity.id;

                meshEntity.addComponent<MeshRendererComponent>(
                    part.mesh.get(),
                    resolvePartMaterial(part)
                );

                moveEntity(meshEntityId, nodeEntityId, partOrder, true);
            }
        }
    }

    return rootEntityId;
}
