#pragma once

#include <optional>
#include <vector>

#include <glm/mat4x4.hpp>

#include "../scene/EntityId.h"
#include "mesh/InstancedMeshRendererComponent.h"
#include "mesh/MeshRendererComponent.h"

class Entity;

struct RenderItem {
    const Entity *entity = nullptr;
    const MeshRendererComponent *meshRenderer = nullptr;
    glm::mat4 worldMatrix{1.0f};
    float cameraDistanceSquared = 0.0f;
    std::optional<OutlineVisibility> outlineVisibility;
};

struct XRayOutlineGroup {
    EntityId id;
    std::vector<RenderItem> items;
};

struct InstancedRenderItem {
    const Entity *entity = nullptr;
    const InstancedMeshRendererComponent *meshRenderer = nullptr;
    glm::mat4 worldMatrix{1.0f};
};

struct RenderQueue {
    std::vector<RenderItem> opaque;
    std::vector<InstancedRenderItem> instancedOpaque;
    std::vector<RenderItem> alphaCutout;
    std::vector<RenderItem> transparent;
    std::vector<RenderItem> normalDebug;
    std::vector<RenderItem> visibleOutlines;
    std::vector<XRayOutlineGroup> xRayOutlineGroups;
};
