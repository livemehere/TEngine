#pragma once

#include "../MeshData.h"

namespace PrimitiveMeshes {
    MeshData createPlane();
    MeshData createCube();
    MeshData createSphere(unsigned int segments = 48, unsigned int rings = 24);
}
