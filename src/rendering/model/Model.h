#pragma once

#include <string>
#include <assimp/scene.h>

#include "../mesh/Mesh.h"

class Model {
    void processNode(const aiNode* node, const aiScene *scene);
    std::unique_ptr<Mesh> processMesh(const aiMesh *mesh, const aiScene *scene);
public:
    std::vector<std::unique_ptr<Mesh>> meshes;

    Model(const std::string& path);
    ~Model() = default;
};
