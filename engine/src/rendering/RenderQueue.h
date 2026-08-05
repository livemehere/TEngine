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
    const Mesh *mesh = nullptr;
    const Material *material = nullptr;
    glm::mat4 worldMatrix{1.0f};
    std::vector<glm::mat4> localMatrices;
};

struct ShadowBatch {
    const Mesh *mesh = nullptr;
    CullMode cullMode = CullMode::Back;
    std::vector<glm::mat4> worldMatrices;
};

struct RenderQueue {
    std::vector<RenderItem> opaque;
    std::vector<InstancedRenderItem> instancedOpaque;
    std::vector<RenderItem> alphaCutout;
    std::vector<RenderItem> transparent;
    std::vector<RenderItem> normalDebug;
    std::vector<RenderItem> visibleOutlines;
    std::vector<XRayOutlineGroup> xRayOutlineGroups;
    std::vector<ShadowBatch> shadowBatches;
};
