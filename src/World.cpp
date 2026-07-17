#include "common.h"
#include "World.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "opengl/Shader.h"

World::World()  {
    // shader
    shader.use();
    modelLocation = glGetUniformLocation(shader.getId(), "uModel");
    viewLocation = glGetUniformLocation(shader.getId(), "uView");
    projectionLocation = glGetUniformLocation(shader.getId(), "uProjection");


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
        .texture = &texture
    });

    objects.push_back({
    .transform = {
        .position = {1.0f,0.0f,0.0f},
        .rotation = {0.0f,0.0f,0.0f},
        .scale = {1.0f,1.0f,1.0f},
    },
    .mesh = &quadMesh,
    .shader = &shader,
    .texture = &texture
});

}

World::~World() {

}

void World::update(const glm::mat4& view, const glm::mat4& projection) {

    shader.use();
    texture.bind(0);

    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

    for (auto& obj : objects) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, obj.transform.position);
        glm::mat4 quatMat = glm::mat4_cast(glm::quat(glm::radians(obj.transform.rotation)));
        model *= quatMat;
        model = glm::scale(model, obj.transform.scale);
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
        obj.mesh->draw();
    }


}

void World::render() {
}
