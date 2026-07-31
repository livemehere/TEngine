#pragma once

#include <optional>
#include <string>

#include "../rendering/Transform.h"
#include "../components/MeshRendererComponent.h"
#include "../components/ModelRendererComponent.h"

using EntityId = std::uint64_t;

struct Entity {
    EntityId id;
    std::string name = "Entity";
    Transform localTransform{
        .position = {0.0f, 0.0f, 0.0f},
        .rotation = {0.0f, 0.0f, 0.0f},
        .scale = {1.0f, 1.0f, 1.0f}
    };

    std::optional<EntityId> parentId = std::nullopt;
    size_t siblingIndex = 0;

    std::optional<MeshRendererComponent> meshRenderer;
    std::optional<ModelRendererComponent> modelRenderer;
};

struct EntityMoveRequest {
    EntityId sourceId;
    // std::nullopt = Root
    std::optional<EntityId> newParentId;
    size_t insertIndex;
};
