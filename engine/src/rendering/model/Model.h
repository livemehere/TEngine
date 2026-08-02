#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../Transform.h"
#include "../mesh/Mesh.h"

class Material;

struct ModelNode {
    std::string name;
    std::optional<std::size_t> parentIndex;
    Transform localTransform{
        .position = {0.0f, 0.0f, 0.0f},
        .rotation = {0.0f, 0.0f, 0.0f},
        .scale = {1.0f, 1.0f, 1.0f}
    };
    std::vector<size_t> partIndices;
};

struct ModelPart {
    std::unique_ptr<Mesh> mesh;
    size_t materialSlot = 0;
};

struct Model {
    std::vector<ModelNode> nodes;
    std::vector<ModelPart> parts;
    std::vector<const Material*> materials;
};
