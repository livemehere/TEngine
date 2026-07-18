#include "common.h"
#include "World.h"
#include "./rendering/Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "graphics/Shader.h"

World::World(ResourceManager& resourceManager) : resourceManager_(resourceManager)  {
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // render as wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Back-face culling (enable for one-sided meshes)
    // NOTE: keep disabled when both sides of a surface must be visible.
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    /* setup objects */
    meshObjects.push_back({
        .transform = {
            .position = {0.0f,0.0f,0.0f},
            .rotation = {-90.0f,0.0f,0.0f},
            .scale = {5.0f,5.0f,5.0f},
        },
        .mesh = &resourceManager.getPlaneMesh(),
        .material = &white,
    });

    meshObjects.push_back({
       .transform = {
           .position = {0.0f,1.1f,0.0f},
           .rotation = {0.0f,0.0f,0.0f},
           .scale = {1.0f,1.0f,1.0f},
       },
       .mesh = &resourceManager.getCubeMesh(),
       .material = &green,
   });
}

void World::update(float dt) {
    for (auto& obj : meshObjects) {
    }
}

void World::render(const RenderContext& context) {
    for (auto& obj : meshObjects) {
        meshRenderer.render(context, obj);
    }
}
