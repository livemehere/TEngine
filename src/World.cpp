#include "common.h"
#include "World.h"

#include <imgui.h>
#include <vector>

#include "utils.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "opengl/ShaderProgram.h"

World::World()  {
    // VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // EBO
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // layout
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(1);

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

}

World::~World() {
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
}


void World::update(const glm::mat4& view, const glm::mat4& projection) {

    // for 3D
    transform.position.z = -1.0f;

    /* MODEL */
    model = glm::mat4(1.0f);

    // position
    model = glm::translate(model, transform.position);

    // rotation

    model = glm::rotate(model,glm::radians(transform.rotation.x),glm::vec3(1.0f,0.0f,0.0f));
    model = glm::rotate(model,glm::radians(transform.rotation.y),glm::vec3(0.0f,1.0f,0.0f));
    model = glm::rotate(model,glm::radians(transform.rotation.z),glm::vec3(0.0f,0.0f,1.0f));

    // scale
    model = glm::scale(model, transform.scale);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

}

void World::render() {
    texture.bind(0);
    shader.use();
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(),GL_UNSIGNED_INT,0);


}
