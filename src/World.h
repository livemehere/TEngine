#pragma once

#include <vector>
#include <glad/glad.h>

#include <glm/glm.hpp>

#include "rendering/mesh/Mesh.h"
#include "graphics/Shader.h"
#include "graphics/Texture2D.h"
#include "rendering/mesh/MeshRenderer.h"
#include "rendering/mesh/MeshRenderObject.h"
#include "rendering/mesh/materials/BasicMaterial.h"

constexpr std::array<uint8_t,4> pixels = {
    255,255,255,255,
};

class World {
private:
    Mesh quadMesh{
         std::array{
             Vertex{{ -0.5f, -0.5f, 0.0f}, {0.0f,0.0f}}, // left bottom
             Vertex{{ 0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}}, // right bottom
             Vertex{{ -0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}}, // left top
             Vertex{{ 0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}}, // right top
         },
        std::array<GLuint, 6>{
            0, 1, 2,
            1, 3, 2
         }
    };
    Shader shader{"shaders/basic.vert", "shaders/basic.frag"};
    Texture2D texture{1,1,pixels};
    BasicMaterial white{shader,texture};
    BasicMaterial green{shader,texture, {0.0f, 0.9f, 0.0f,1.0f}};

    MeshRenderer meshRenderer;
    std::vector<MeshRenderObject> meshObjects;

public:
    World();
    ~World() = default;

    void update(float dt);
    void render(const RenderContext& context);
};
