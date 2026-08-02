#pragma once

enum class DebugViewMode : int {
    Shaded = 0,
    Depth,
    WorldNormal
};

enum class RasterizationMode : int {
    Fill = 0,
    Wireframe
};

struct RenderSettings {
    DebugViewMode debugView = DebugViewMode::Shaded;
    RasterizationMode rasterization = RasterizationMode::Fill;
    float debugDepthNear = 0.0f;
    float debugDepthFar = 30.0f;
};
