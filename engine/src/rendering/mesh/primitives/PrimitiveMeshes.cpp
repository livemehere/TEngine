#include "../../mesh/primitives/PrimitiveMeshes.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace PrimitiveMeshes {
    MeshData createPlane() {
        return {
            .vertices = {
                {{ -0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f,0.0f}}, // BL
                {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, // BR
                {{ -0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // TL
                {{ 0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // TR
            },
            .indices = {
                0, 1, 2,
                1, 3, 2
            }
        };
    }

    MeshData createCube() {
        std::vector<GLuint> indices;

        const int faceCount = 6;
        const int indexGap = 4;

        indices.reserve(faceCount * 6);
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
                {{ -0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f,0.0f}}, // BL
                {{ 0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, // BR
                {{ -0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // TL
                {{ 0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // TR
                // back
                {{ 0.5f, -0.5f, -0.5f},{0.0f, 0.0f, -1.0f}, {0.0f,0.0f}}, // BL
                {{ -0.5f, -0.5f, -0.5f},{0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}}, // BR
                {{ 0.5f, 0.5f, -0.5f},{0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}}, // TL
                {{ -0.5f, 0.5f, -0.5f},{0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}}, // TR
                // bottom
                {{ -0.5f, -0.5f, -0.5f},{0.0f, -1.0f, 0.0f}, {0.0f,0.0f}}, // BL
                {{ 0.5f, -0.5f, -0.5f},{0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}}, // BR
                {{ -0.5f, -0.5f, 0.5f},{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}}, // TL
                {{ 0.5f, -0.5f, 0.5f},{0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}}, // TR
                // top
                {{ -0.5f, 0.5f, 0.5f},{0.0f, 1.0f, 0.0f}, {0.0f,0.0f}}, // BL
                {{ 0.5f, 0.5f, 0.5f},{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // BR
                {{ -0.5f, 0.5f, -0.5f},{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}, // TL
                {{ 0.5f, 0.5f, -0.5f},{0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}, // TR
                // left
                {{ -0.5f, -0.5f, -0.5f},{-1.0f, 0.0f, 0.0f}, {0.0f,0.0f}}, // BL
                {{ -0.5f, -0.5f, 0.5f},{-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // BR
                {{ -0.5f, 0.5f, -0.5f},{-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // TL
                {{ -0.5f, 0.5f, 0.5f},{-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}, // TR
                // right
               {{ 0.5f, -0.5f, 0.5f},{1.0f, 0.0f, 0.0f}, {0.0f,0.0f}}, // BL
               {{ 0.5f, -0.5f, -0.5f},{1.0f, 0.0f, 0.0f},  {1.0f, 0.0f}}, // BR
               {{ 0.5f, 0.5f, 0.5f},{1.0f, 0.0f, 0.0f},  {0.0f, 1.0f}}, // TL
               {{ 0.5f, 0.5f, -0.5f},{1.0f, 0.0f, 0.0f},  {1.0f, 1.0f}}, // TR
},
            .indices = std::move(indices)
        };
    }

    MeshData createSphere(const unsigned int segments, const unsigned int rings) {
        MeshData result;
        const unsigned int safeSegments = std::max(segments, 3u);
        const unsigned int safeRings = std::max(rings, 2u);
        result.vertices.reserve(
            static_cast<std::size_t>(safeSegments + 1) *
            static_cast<std::size_t>(safeRings + 1)
        );
        result.indices.reserve(
            static_cast<std::size_t>(safeSegments) *
            static_cast<std::size_t>(safeRings) * 6
        );

        for (unsigned int ring = 0; ring <= safeRings; ++ring) {
            const float v = static_cast<float>(ring) /
                            static_cast<float>(safeRings);
            const float phi = v * std::numbers::pi_v<float>;
            const float sinPhi = std::sin(phi);
            const float cosPhi = std::cos(phi);

            for (unsigned int segment = 0;
                 segment <= safeSegments;
                 ++segment) {
                const float u = static_cast<float>(segment) /
                                static_cast<float>(safeSegments);
                const float theta = u * std::numbers::pi_v<float> * 2.0f;
                const glm::vec3 normal{
                    sinPhi * std::cos(theta),
                    cosPhi,
                    sinPhi * std::sin(theta)
                };
                result.vertices.push_back({
                    .position = normal * 0.5f,
                    .normal = normal,
                    .texCoord = {u, 1.0f - v}
                });
            }
        }

        const unsigned int rowWidth = safeSegments + 1;
        for (unsigned int ring = 0; ring < safeRings; ++ring) {
            for (unsigned int segment = 0;
                 segment < safeSegments;
                 ++segment) {
                const GLuint topLeft = ring * rowWidth + segment;
                const GLuint bottomLeft = topLeft + rowWidth;
                result.indices.insert(
                    result.indices.end(),
                    {
                        topLeft,
                        bottomLeft,
                        topLeft + 1,
                        topLeft + 1,
                        bottomLeft,
                        bottomLeft + 1
                    }
                );
            }
        }
        return result;
    }
}
