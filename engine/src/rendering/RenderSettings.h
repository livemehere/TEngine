#pragma once

enum class DebugViewMode : int {
    Shaded = 0,
    Depth,
    WorldNormal,
    GBufferPosition,
    GBufferAlbedo,
    GBufferSpecular
};

enum class RasterizationMode : int {
    Fill = 0,
    Wireframe
};

enum class RenderingPath : int {
    Forward = 0,
    Deferred
};

enum class ToneMappingMode : int {
    Reinhard = 0,
    Exposure
};

struct RenderSettings {
    DebugViewMode debugView = DebugViewMode::Shaded;
    RenderingPath renderingPath = RenderingPath::Deferred;
    RasterizationMode rasterization = RasterizationMode::Fill;
    float debugDepthNear = 0.0f;
    float debugDepthFar = 30.0f;
    int msaaSamples = 1;
    bool hdrEnabled = true;
    ToneMappingMode toneMapping = ToneMappingMode::Exposure;
    float exposure = 1.0f;
    bool bloomEnabled = true;
    float bloomThreshold = 1.0f;
    float bloomStrength = 0.15f;
    int bloomBlurPasses = 10;
    bool gammaCorrection = true;
    float gamma = 2.2f;
    bool shadowsEnabled = true;
    int shadowMapResolution = 2048;
    float shadowDistance = 25.0f;
    float shadowBiasMin = 0.0005f;
    float shadowBiasSlope = 0.005f;
    int shadowPcfRadius = 1;
    bool pointShadowsEnabled = true;
    int pointShadowMapResolution = 1024;
    float pointShadowBias = 0.05f;
    float pointShadowSoftness = 0.04f;
    int pointShadowSampleCount = 20;
};
