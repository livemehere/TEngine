#pragma once

#include <string>
#include <assimp/scene.h>

#include "../Transform.h"
#include "../mesh/Mesh.h"
#include "../mesh/materials/Material.h"

struct ModelNode {
    std::string name;
    int parentIndex = -1;
    Transform localTransform;
    std::vector<size_t> partIndices;
};

struct ModelPart {
    std::unique_ptr<Mesh> mesh;
    size_t materialSlot = 0;
};

class Model {
    std::vector<ModelNode> nodes;
    std::vector<ModelPart> parts;
    std::vector<const Material*> materials;

    void processNode(const aiNode *node, const aiScene *scene, int parentIndex);
    std::unique_ptr<Mesh> processMesh(const aiMesh *mesh, const aiScene *scene);
public:
    Model(const std::string &path, bool flipUVs);
    ~Model() = default;

    const std::vector<ModelPart> &getParts() const { return parts; }

    // https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/assimp_glm_helpers.h
    static glm::mat4 convertMatrixToGlmFormat(const aiMatrix4x4 &from) {
        glm::mat4 to;
        //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
        to[0][0] = from.a1;
        to[1][0] = from.a2;
        to[2][0] = from.a3;
        to[3][0] = from.a4;
        to[0][1] = from.b1;
        to[1][1] = from.b2;
        to[2][1] = from.b3;
        to[3][1] = from.b4;
        to[0][2] = from.c1;
        to[1][2] = from.c2;
        to[2][2] = from.c3;
        to[3][2] = from.c4;
        to[0][3] = from.d1;
        to[1][3] = from.d2;
        to[2][3] = from.d3;
        to[3][3] = from.d4;
        return to;
    }
};
