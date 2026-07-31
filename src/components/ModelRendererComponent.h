#pragma once
#include "../rendering/mesh/materials/Material.h"
#include "../rendering/model/Model.h"

struct ModelRendererComponent {
    const Model* model = nullptr;
    const Material* material = nullptr;
};
