#include "../../mesh/primitives/PrimitiveMeshes.h"

namespace PrimitiveMeshes {
    MeshData createPlane() {
        return {
            .vertices = {
                {{ -0.5f, -0.5f, 0.0f}, {0.0f,0.0f}}, // BL
                {{ 0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}}, // BR
                {{ -0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}}, // TL
                {{ 0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}}, // TR
            },
            .indices = {
                0, 1, 2,
                1, 3, 2
            }
        };
    }

    MeshData createCube() {
        std::vector<GLuint> indices;

        const int faceCount = 3;
        const int indexGap = 4;
        for (int i = 0; i < faceCount * indexGap; i += indexGap) {
           indices.push_back(i);
           indices.push_back(i + 1);
           indices.push_back(i + 2);

           indices.push_back(i + 1);
           indices.push_back(i + 3);
           indices.push_back(i + 2);
        }

        return {
            .vertices = {
                // front
                {{ -0.5f, -0.5f, 0.5f}, {0.0f,0.0f}}, // BL
                {{ 0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}}, // BR
                {{ -0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}}, // TL
                {{ 0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}}, // TR
                // back
                {{ -0.5f, -0.5f, -0.5f}, {0.0f,0.0f}}, // BL
                {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}}, // BR
                {{ -0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}}, // TL
                {{ 0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}}, // TR
                // bottom
                {{ -0.5f, -0.5f, -0.5f}, {0.0f,0.0f}}, // BL
                {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}}, // BR
                {{ -0.5f, -0.5f, 0.5f}, {0.0f, 1.0f}}, // TL
                {{ 0.5f, -0.5f, 0.5f}, {1.0f, 1.0f}}, // TR
},
            .indices = std::move(indices)
        };
    }
}
