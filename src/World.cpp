#include "World.h"

#include <iostream>
#include <vector>
#include "utils.h"

World::World() {
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    auto vsSrc = utils::read_file(utils::asset_path("shaders/basic.vert"));
    std::cout << vsSrc << "\n";

}

World::~World() {
}

void World::update() {
}

void World::render() {


}
