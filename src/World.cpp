#include "common.h"
#include "World.h"
#include "./renderer/Transform.h"
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
        .shader = &shader,
        .texture = &texture,
        .color = glm::vec4{1.0f, 0.0f, 0.0f, 1.0f}
    });

    objects.push_back({
    .transform = {
        .position = {1.0f,0.0f,0.0f},
        .rotation = {0.0f,0.0f,0.0f},
        .scale = {1.0f,1.0f,1.0f},
    },
    .mesh = &quadMesh,
    .shader = &shader,
    .texture = &texture,
    .color = glm::vec4{0.8f, 0.8f, 0.0f, 1.0f}
});

}

World::~World() {

}

void World::update(const glm::mat4& view, const glm::mat4& projection) {

    for (auto& obj : objects) {
        obj.texture->bind(0);
        obj.shader->use();
        obj.shader->setInt("uTexture",0);
        obj.shader->setVec4("uColor",obj.color);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, obj.transform.position);
        glm::mat4 quatMat = glm::mat4_cast(glm::quat(glm::radians(obj.transform.rotation)));
        model *= quatMat;
        model = glm::scale(model, obj.transform.scale);
        obj.shader->setMat4("uModel",model);
        obj.shader->setMat4("uView",view);
        obj.shader->setMat4("uProjection",projection);
        obj.mesh->draw();
    }


}

void World::render() {
}
