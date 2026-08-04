#include "ComponentTypeRegistry.h"

#include "../camera/CameraComponent.h"
#include "../rendering/Lights.h"
#include "../rendering/mesh/MeshRendererComponent.h"
#include "../rendering/mesh/InstancedMeshRendererComponent.h"
#include "../rendering/model/InstancedModelRendererComponent.h"
#include "../rendering/skybox/SkyboxComponent.h"

void ComponentTypeRegistry::addDescriptor(ComponentTypeDescriptor descriptor) {
    if (find(descriptor.type)) {
        throw std::logic_error("Component type is already registered");
    }
    descriptors.push_back(std::move(descriptor));
}

const ComponentTypeDescriptor *ComponentTypeRegistry::find(
    std::type_index type
) const {
    for (const ComponentTypeDescriptor &descriptor: descriptors) {
        if (descriptor.type == type) {
            return &descriptor;
        }
    }
    return nullptr;
}

void registerBuiltinComponentTypes(ComponentTypeRegistry &registry) {
    registry.registerComponent<MeshRendererComponent>("Mesh Renderer");
    registry.registerComponent<InstancedMeshRendererComponent>(
        "Instanced Mesh Renderer"
    );
    registry.registerComponent<InstancedModelRendererComponent>(
        "Instanced Model Renderer"
    );
    registry.registerComponent<CameraComponent>("Camera");
    registry.registerComponent<AmbientLightComponent>("Ambient Light");
    registry.registerComponent<DirectionalLightComponent>("Directional Light");
    registry.registerComponent<PointLightComponent>("Point Light");
    registry.registerComponent<SpotLightComponent>("Spot Light");
    registry.registerComponent<SkyboxComponent>("Skybox");
}
