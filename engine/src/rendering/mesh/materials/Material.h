#pragma once
#include "../../../graphics/Shader.h"

enum class CullMode {
    None,
    Back,
    Front
};

enum class RenderQueueType {
    Opaque,
    AlphaCutout,
    Transparent
};

struct RasterState {
    CullMode cullMode = CullMode::Back;
};

class Material {
public:
    const Shader& shader;
    RasterState rasterState;
    RenderQueueType renderQueue = RenderQueueType::Opaque;

    Material(const Shader& shader) : shader(shader) {}

    virtual ~Material() = default;

    virtual void bind () const = 0;
};
