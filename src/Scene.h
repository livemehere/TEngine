#pragma once

#include <vector>
#include "camera/Camera.h"
#include "rendering/Lights.h"
#include "rendering/mesh/MeshRenderObject.h"

class Scene {
public:
    Camera camera;
    std::vector<MeshRenderObject> meshObjects;

    /* Lights */
    AmbientLight ambientLight;
    std::vector<DirectionalLight> directionalLights;
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;

    Scene() = default;
    ~Scene() = default;

    void update(float dt);
};
