#pragma once

class CubeMap;

struct SkyboxComponent {
    const CubeMap* cubeMap = nullptr;
    bool enabled = true;
};
