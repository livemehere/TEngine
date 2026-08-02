#pragma once

#include <filesystem>

#include <assimp/material.h>
#include <assimp/scene.h>

#include "Model.h"

class ResourceManager;
class Texture2D;

class ModelImporter {
    ResourceManager &resourceManager;

    void processNode(
        Model &model,
        const aiNode *node,
        const aiScene *scene,
        std::optional<std::size_t> parentIndex
    );

    std::unique_ptr<Mesh> processMesh(
        const aiMesh *mesh
    );

    void processMaterials(
        Model &model,
        const aiScene *scene,
        const std::filesystem::path &modelPath
    );

    const Texture2D *loadMaterialTexture(
        const aiMaterial *material,
        aiTextureType textureType,
        const aiScene *scene,
        const std::filesystem::path &modelPath
    );

    // https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/assimp_glm_helpers.h
    static glm::mat4 convertMatrixToGlmFormat(const aiMatrix4x4 &from) {
        glm::mat4 to;
        // In Assimp, a/b/c/d identify rows and 1/2/3/4 identify columns.
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

public:
    ModelImporter(ResourceManager &resourceManager);

    std::unique_ptr<Model> import(const std::filesystem::path &path, bool flipUVs);
};
