#include "Model.h"

#include <format>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Model::Model(const std::string &path) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path.c_str(),
        aiProcess_Triangulate
        | aiProcess_FlipUVs
        // | aiProcess_PreTransformVertices
        );
    if (!scene) {
        throw std::runtime_error(std::format("Model load failed {}", path ));
    }

    processNode(scene->mRootNode, scene);
}

void Model::processNode(const aiNode *node, const aiScene *scene) {

    for (int i = 0; i < node->mNumMeshes; i++) {
        const aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    for (int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }

}

std::unique_ptr<Mesh> Model::processMesh(const aiMesh *mesh, const aiScene *scene) {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    /* vertex */
    for (size_t i=0; i<mesh->mNumVertices; i++) {
        glm::vec3 position;
        position.x = mesh->mVertices[i].x;
        position.y = mesh->mVertices[i].y;
        position.z = mesh->mVertices[i].z;

        glm::vec3 normal{0.0f, 1.0f, 0.0f};
        if (mesh->HasNormals()) {
            normal.x = mesh->mNormals[i].x;
            normal.y = mesh->mNormals[i].y;
            normal.z = mesh->mNormals[i].z;
        }

        glm::vec2 texCoord{0.0f,0.0f};
        if (mesh->HasTextureCoords(0)) {
            texCoord.x = mesh->mTextureCoords[0][i].x;
            texCoord.y = mesh->mTextureCoords[0][i].y;
        }
        Vertex vertex(position, normal, texCoord);
        vertices.push_back(vertex);
    }

    /* indices */
    for (size_t i=0; i<mesh->mNumFaces; i++) {
        const aiFace& face = mesh->mFaces[i];
        for (size_t j=0; j<face.mNumIndices;j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    return std::make_unique<Mesh>(vertices, indices);
}
