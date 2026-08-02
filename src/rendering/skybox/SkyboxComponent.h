#pragma once

#include "../../scene/Component.h"

class CubeMap;

class SkyboxComponent final : public Component {
public:
    const CubeMap* cubeMap = nullptr;

    SkyboxComponent() = default;
    explicit SkyboxComponent(const CubeMap* cubeMap) : cubeMap(cubeMap) {}
};
