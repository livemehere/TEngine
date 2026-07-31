#include "Model.h"

#include <format>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Model::Model(const std::string &path, bool flipUVs) {
    Assimp::Importer importer;

    unsigned int pFlags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality |
                          aiProcess_GenSmoothNormals;
    if (flipUVs) {
        pFlags |= aiProcess_FlipUVs;
    }

    const aiScene *scene = importer.ReadFile(path.c_str(), pFlags);

    if (!scene) {
        throw std::runtime_error(std::format("Model load failed {}", path));
    }

    processNode(scene->mRootNode, scene, -1);
}

void Model::processNode(const aiNode *node, const aiScene *scene, int parentIndex) {
    const glm::mat4 localMatrix = convertMatrixToGlmFormat(node->mTransformation);
    Transform localTransform{
        .position = {0.0f, 0.0f, 0.0f},
        .rotation = {0.0f, 0.0f, 0.0f},
        .scale = {1.0f, 1.0f, 1.0f},
    };

    if (!Transform::decompose(localMatrix, localTransform)) {
        throw std::runtime_error(std::format("Failed to decompose model node {}", node->mName.C_Str()));
    }

    size_t nodeIndex = nodes.size();

    nodes.push_back({
        .name = node->mName.length > 0 ? node->mName.C_Str() : "UnnamedNode",
        .parentIndex = parentIndex,
        .localTransform = localTransform,
        .partIndices = {}
    });

    for (int i = 0; i < node->mNumMeshes; i++) {
        const unsigned int meshIndex = node->mMeshes[i];
        const aiMesh *mesh = scene->mMeshes[meshIndex];
        const size_t partIndex = parts.size();

        parts.push_back({
            .mesh = processMesh(mesh, scene),
            .materialSlot = mesh->mMaterialIndex
        });
        nodes[nodeIndex].partIndices.push_back(partIndex);
    }

    for (int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, nodeIndex);
    }
}

std::unique_ptr<Mesh> Model::processMesh(const aiMesh *mesh, const aiScene *scene) {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    /* vertex */
    for (size_t i = 0; i < mesh->mNumVertices; i++) {
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

        glm::vec2 texCoord{0.0f, 0.0f};
        if (mesh->HasTextureCoords(0)) {
            texCoord.x = mesh->mTextureCoords[0][i].x;
            texCoord.y = mesh->mTextureCoords[0][i].y;
        }
        vertices.emplace_back(position, normal, texCoord);
    }

    /* indices */
    for (size_t i = 0; i < mesh->mNumFaces; i++) {
        const aiFace &face = mesh->mFaces[i];
        for (size_t j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    return std::make_unique<Mesh>(vertices, indices);
}
