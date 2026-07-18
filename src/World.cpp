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
    objects.push_back({
        .transform = {
            .position = {-1.0f,0.0f,0.0f},
            .rotation = {0.0f,0.0f,0.0f},
            .scale = {1.0f,1.0f,1.0f},
        },
        .mesh = &quadMesh,
        .material = &white,
    });

    objects.push_back({
    .transform = {
        .position = {1.0f,0.0f,0.0f},
        .rotation = {0.0f,0.0f,0.0f},
        .scale = {1.0f,1.0f,1.0f},
    },
    .mesh = &quadMesh,
    .material = &green,
});

}

World::~World() {

}

void World::update(const glm::mat4& view, const glm::mat4& projection) {

    RenderContext ctx {
        .view = view,
        .projection = projection
    };

    for (auto& obj : objects) {
        obj.material->bind(ctx);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, obj.transform.position);
        glm::mat4 quatMat = glm::mat4_cast(glm::quat(glm::radians(obj.transform.rotation)));
        model *= quatMat;
        model = glm::scale(model, obj.transform.scale);
        obj.material->shader.setMat4("uModel",model);

        obj.mesh->draw();
    }


}

void World::render() {
}
