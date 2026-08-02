#pragma once

#include "../core/Input.h"
#include "../rendering/Transform.h"

class FreeLookCameraController {
public:
    float speed = 2.0f;
    float sprintSpeed = 10.0f;
    float hSensitivity = 0.05f;
    float vSensitivity = 0.05f;

    FreeLookCameraController() = default;
    ~FreeLookCameraController() = default;

    void update(Transform& transform, Input& input, float dt);
};
