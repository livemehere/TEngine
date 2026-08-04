#pragma once

#include "../rendering/mesh/InstanceData.h"
#include "../scene/EntityId.h"

enum class InstanceRendererType {
    Mesh,
    Model
};

struct InstanceSelection {
    EntityId entityId = 0;
    InstanceId instanceId = 0;
    InstanceRendererType rendererType = InstanceRendererType::Mesh;
};
