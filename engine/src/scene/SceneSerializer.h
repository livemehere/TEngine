#pragma once

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <vector>

#include "EntityId.h"
#include "../rendering/Transform.h"

class ComponentTypeRegistry;
class Scene;

struct SerializedComponent {
    std::type_index type{typeid(void)};
    std::any data;
};

struct SerializedEntity {
    EntityId id = 0;
    std::string name;
    std::optional<EntityId> parentId;
    size_t siblingIndex = 0;
    bool transformEnabled = true;
    Transform localTransform;
    std::vector<SerializedComponent> components;
};

struct SerializedScene {
    EntityId nextEntityId = 0;
    std::optional<EntityId> activeCameraId;
    std::vector<SerializedEntity> entities;
};

class SceneSerializer {
public:
    [[nodiscard]] static SerializedScene serialize(
        const Scene &scene,
        const ComponentTypeRegistry &componentTypes
    );

    [[nodiscard]] static std::unique_ptr<Scene> deserialize(
        const SerializedScene &data,
        const ComponentTypeRegistry &componentTypes
    );

    [[nodiscard]] static std::unique_ptr<Scene> cloneForRuntime(
        const Scene &scene,
        const ComponentTypeRegistry &componentTypes
    );
};
