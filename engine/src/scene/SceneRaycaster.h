#pragma once

#include <optional>

#include <glm/vec3.hpp>

#include "EntityId.h"
#include "../rendering/mesh/InstanceData.h"

class Scene;

struct Ray {
    glm::vec3 origin{0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};
};

enum class SceneRaycastTarget {
    Entity,
    MeshInstance,
    ModelInstance
};

struct SceneRaycastHit {
    EntityId entityId = 0;
    SceneRaycastTarget target = SceneRaycastTarget::Entity;
    std::optional<InstanceId> instanceId;
    float distance = 0.0f;
};

class SceneRaycaster {
public:
    [[nodiscard]] static std::optional<SceneRaycastHit> cast(
        const Scene &scene,
        const Ray &ray
    );
};
