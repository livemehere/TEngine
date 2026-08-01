#pragma once
#include "../../../graphics/Shader.h"

enum class CullMode {
    None,
    Back,
    Front
};

struct RasterState {
    CullMode cullMode = CullMode::Back;
};

class Material {
public:
    const Shader& shader;
    RasterState rasterState;

    Material(const Shader& shader) : shader(shader) {}

    virtual ~Material() = default;

    virtual void bind () const = 0;
};
