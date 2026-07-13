#include "World.h"

#include <iostream>
#include <vector>
#include "utils.h"

World::World() {
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // shader
     GLint vs = utils::compile_shader(GL_VERTEX_SHADER,"shaders/basic.vert");


}

World::~World() {
}

void World::update() {
}

void World::render() {


}
