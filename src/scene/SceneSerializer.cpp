#include "SceneSerializer.h"

#include <format>
#include <stdexcept>

#include "ComponentTypeRegistry.h"
#include "Scene.h"
#include "TransformComponent.h"

SerializedScene SceneSerializer::serialize(
    const Scene &scene,
    const ComponentTypeRegistry &componentTypes
) {
    SerializedScene result{
        .nextEntityId = scene.entitySeq,
        .activeCameraId = scene.activeCameraId
    };
    result.entities.reserve(scene.entities.size());

    for (const Entity &entity: scene.entities) {
        const TransformComponent &transform =
                entity.getComponent<TransformComponent>();
        SerializedEntity serializedEntity{
            .id = entity.id,
            .name = entity.name,
            .parentId = entity.parentId,
            .siblingIndex = entity.siblingIndex,
            .transformEnabled = transform.enabled,
            .localTransform = transform.local
        };

        for (const auto &[type, component]: entity.components_) {
            if (type == std::type_index(typeid(TransformComponent))) {
                continue;
            }

            const ComponentTypeDescriptor *descriptor = componentTypes.find(type);
            if (!descriptor) {
                throw std::logic_error(std::format(
                    "Cannot serialize unregistered component type on entity {}",
                    entity.id
                ));
            }

            serializedEntity.components.push_back({
                .type = type,
                .data = descriptor->serialize(*component)
            });
        }

        result.entities.push_back(std::move(serializedEntity));
    }

    return result;
}

std::unique_ptr<Scene> SceneSerializer::deserialize(
    const SerializedScene &data,
    const ComponentTypeRegistry &componentTypes
) {
    auto result = std::make_unique<Scene>();
    result->entitySeq = data.nextEntityId;
    result->activeCameraId = data.activeCameraId;

    for (const SerializedEntity &serializedEntity: data.entities) {
        Entity &entity = result->entities.emplace_back(
            *result,
            result->componentIterationDepth,
            serializedEntity.id,
            serializedEntity.name,
            serializedEntity.siblingIndex
        );
        entity.parentId = serializedEntity.parentId;

        TransformComponent &transform =
                entity.getComponent<TransformComponent>();
        transform.enabled = serializedEntity.transformEnabled;
        transform.local = serializedEntity.localTransform;
    }

    std::vector<Component *> componentsToAttach;
    for (size_t entityIndex = 0; entityIndex < data.entities.size(); ++entityIndex) {
        Entity &entity = result->entities[entityIndex];

        for (const SerializedComponent &serializedComponent:
             data.entities[entityIndex].components) {
            const ComponentTypeDescriptor *descriptor =
                    componentTypes.find(serializedComponent.type);
            if (!descriptor) {
                throw std::logic_error(std::format(
                    "Cannot deserialize unregistered component type on entity {}",
                    entity.id
                ));
            }

            std::unique_ptr<Component> component =
                    descriptor->instantiate(serializedComponent.data);
            if (!component) {
                throw std::logic_error("Component factory returned null");
            }

            Component *attachedComponent = component.get();
            attachedComponent->attachTo(*result, entity.id);
            entity.components_.emplace(
                serializedComponent.type,
                std::move(component)
            );
            componentsToAttach.push_back(attachedComponent);
        }
    }

    for (Component *component: componentsToAttach) {
        component->onAttach();
    }

    return result;
}

std::unique_ptr<Scene> SceneSerializer::cloneForRuntime(
    const Scene &scene,
    const ComponentTypeRegistry &componentTypes
) {
    return deserialize(serialize(scene, componentTypes), componentTypes);
}
