#pragma once

#include "mesh/Mesh.h"
#include "../rendering/Transform.h"
#include "mesh/materials/Material.h"

struct RenderObject {
    Transform transform;
    Mesh* mesh;
    Material* material;
};

