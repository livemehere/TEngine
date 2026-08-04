#pragma once

#include "../../scene/Component.h"
#include "../mesh/InstanceData.h"
#include "Model.h"

class InstancedModelRendererComponent final : public Component {
public:
    const Model *model = nullptr;
    const Material *fallbackMaterial = nullptr;
    InstanceCollection instances;

    InstancedModelRendererComponent() = default;

    InstancedModelRendererComponent(
        const Model *model,
        const Material *fallbackMaterial = nullptr
    ) : model(model), fallbackMaterial(fallbackMaterial) {}

    [[nodiscard]] InstanceId addInstance(const Transform &transform = {}) {
        return instances.add(transform);
    }

    [[nodiscard]] bool removeInstance(InstanceId id) {
        return instances.remove(id);
    }

    [[nodiscard]] InstanceData *findInstance(InstanceId id) {
        return instances.find(id);
    }

    [[nodiscard]] const InstanceData *findInstance(InstanceId id) const {
        return instances.find(id);
    }
};
