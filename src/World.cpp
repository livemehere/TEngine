#include "common.h"
#include "World.h"
#include "./rendering/Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "graphics/Shader.h"

World::World()  {
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // render as wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // disable back-face
    // NOTE: if two size object needed, turn of culling
    // glEnable(GL_CULL_FACE);
    // glFrontFace(GL_CCW);
    // glCullFace(GL_BACK);

    /* setup objects */
    meshObjects.push_back({
        .transform = {
            .position = {-1.0f,0.0f,0.0f},
            .rotation = {0.0f,0.0f,0.0f},
            .scale = {1.0f,1.0f,1.0f},
        },
        .mesh = &quadMesh,
        .material = &white,
    });

    meshObjects.push_back({
    .transform = {
        .position = {1.0f,0.0f,0.0f},
        .rotation = {0.0f,0.0f,0.0f},
        .scale = {1.0f,1.0f,1.0f},
    },
    .mesh = &quadMesh,
    .material = &green,
});

}

void World::update(float dt) {
    for (auto& obj : meshObjects) {
        // update game logic
    }
}

void World::render(const RenderContext& context) {
    for (auto& obj : meshObjects) {
        meshRenderer.render(context, obj);
    }
}
