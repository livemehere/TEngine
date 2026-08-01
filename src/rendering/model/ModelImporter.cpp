#include "ModelImporter.h"

#include <cstdint>
#include <format>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/texture.h>

#include "../../common.h"
#include "../../resources/ResourceManager.h"
#include "../mesh/materials/LitMaterial.h"

ModelImporter::ModelImporter(
    ResourceManager &resourceManager
) : resourceManager(resourceManager) {
}

std::unique_ptr<Model> ModelImporter::import(
    const std::filesystem::path &path,
    bool flipUVs
) {
    Assimp::Importer importer;

    unsigned int flags =
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality |
            aiProcess_GenSmoothNormals;

    if (flipUVs) {
        flags |= aiProcess_FlipUVs;
    }

    const aiScene *scene = importer.ReadFile(
        path.string(),
        flags
    );

    if (!scene ||
        (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) ||
        !scene->mRootNode) {
        throw std::runtime_error(std::format(
            "Model import failed '{}': {}",
            path.string(),
            importer.GetErrorString()
        ));
    }

    auto model = std::make_unique<Model>();

    processNode(
        *model,
        scene->mRootNode,
        scene,
        std::nullopt
    );

    processMaterials(
        *model,
        scene,
        path
    );

    return model;
}

void ModelImporter::processNode(
    Model &model,
    const aiNode *node,
    const aiScene *scene,
    std::optional<std::size_t> parentIndex
) {
    const glm::mat4 localMatrix =
            convertMatrixToGlmFormat(node->mTransformation);

    Transform localTransform{
        .position = {0.0f, 0.0f, 0.0f},
        .rotation = {0.0f, 0.0f, 0.0f},
        .scale = {1.0f, 1.0f, 1.0f}
    };

    if (!Transform::decompose(
        localMatrix,
        localTransform
    )) {
        throw std::runtime_error(std::format(
            "Failed to decompose model node '{}'",
            node->mName.C_Str()
        ));
    }

    const std::size_t nodeIndex =
            model.nodes.size();

    model.nodes.push_back({
        .name = node->mName.length > 0
                    ? node->mName.C_Str()
                    : "UnnamedNode",
        .parentIndex = parentIndex,
        .localTransform = localTransform,
        .partIndices = {}
    });

    for (unsigned int i = 0;
         i < node->mNumMeshes;
         ++i) {
        const unsigned int meshIndex =
                node->mMeshes[i];

        const aiMesh *sourceMesh =
                scene->mMeshes[meshIndex];

        const std::size_t partIndex =
                model.parts.size();

        model.parts.push_back({
            .mesh = processMesh(sourceMesh),
            .materialSlot =
            static_cast<std::size_t>(
                sourceMesh->mMaterialIndex
            )
        });

        model.nodes[nodeIndex]
                .partIndices
                .push_back(partIndex);
    }

    for (unsigned int i = 0;
         i < node->mNumChildren;
         ++i) {
        processNode(
            model,
            node->mChildren[i],
            scene,
            nodeIndex
        );
    }
}

std::unique_ptr<Mesh> ModelImporter::processMesh(
    const aiMesh *mesh
) {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    vertices.reserve(mesh->mNumVertices);

    for (unsigned int i = 0;
         i < mesh->mNumVertices;
         ++i) {
        const aiVector3D &sourcePosition =
                mesh->mVertices[i];

        const glm::vec3 position{
            sourcePosition.x,
            sourcePosition.y,
            sourcePosition.z
        };

        glm::vec3 normal{
            0.0f,
            1.0f,
            0.0f
        };

        if (mesh->HasNormals()) {
            const aiVector3D &sourceNormal =
                    mesh->mNormals[i];

            normal = {
                sourceNormal.x,
                sourceNormal.y,
                sourceNormal.z
            };
        }

        glm::vec2 texCoord{
            0.0f,
            0.0f
        };

        if (mesh->HasTextureCoords(0)) {
            const aiVector3D &sourceTexCoord =
                    mesh->mTextureCoords[0][i];

            texCoord = {
                sourceTexCoord.x,
                sourceTexCoord.y
            };
        }

        vertices.emplace_back(
            position,
            normal,
            texCoord
        );
    }

    for (unsigned int i = 0;
         i < mesh->mNumFaces;
         ++i) {
        const aiFace &face =
                mesh->mFaces[i];

        for (unsigned int j = 0;
             j < face.mNumIndices;
             ++j) {
            indices.push_back(
                face.mIndices[j]
            );
        }
    }

    return std::make_unique<Mesh>(
        vertices,
        indices
    );
}

void ModelImporter::processMaterials(
    Model &model,
    const aiScene *scene,
    const std::filesystem::path &modelPath
) {
    model.materials.assign(
        scene->mNumMaterials,
        nullptr
    );

    for (unsigned int materialIndex = 0;
         materialIndex < scene->mNumMaterials;
         ++materialIndex) {
        const aiMaterial *sourceMaterial =
                scene->mMaterials[materialIndex];

        try {
            // glTF/PBR
            const Texture2D *albedoTexture =
                    loadMaterialTexture(
                        sourceMaterial,
                        aiTextureType_BASE_COLOR,
                        scene,
                        modelPath
                    );

            // OBJ/Phong
            if (!albedoTexture) {
                albedoTexture =
                        loadMaterialTexture(
                            sourceMaterial,
                            aiTextureType_DIFFUSE,
                            scene,
                            modelPath
                        );
            }

            // use fallbackMaterial
            if (!albedoTexture) {
                continue;
            }

            const std::string materialKey =
                    std::format(
                        "{}#material:{}",
                        modelPath.string(),
                        materialIndex
                    );

            LitMaterial &importedMaterial =
                    resourceManager.loadLitMaterial(
                        materialKey,
                        resourceManager.getLitShader(),
                        *albedoTexture
                    );

            aiColor4D baseColor{
                1.0f,
                1.0f,
                1.0f,
                1.0f
            };

            if (sourceMaterial->Get(
                    AI_MATKEY_BASE_COLOR,
                    baseColor
                ) != AI_SUCCESS) {
                sourceMaterial->Get(
                    AI_MATKEY_COLOR_DIFFUSE,
                    baseColor
                );
            }

            importedMaterial.baseColor = {
                baseColor.r,
                baseColor.g,
                baseColor.b,
                baseColor.a
            };

            importedMaterial.specularTexture =
                    nullptr;

            try {
                importedMaterial.specularTexture =
                        loadMaterialTexture(
                            sourceMaterial,
                            aiTextureType_SPECULAR,
                            scene,
                            modelPath
                        );
            } catch (const std::exception &e) {
                LOG(std::format(
                    "Specular texture load failed: {}",
                    e.what()
                ));
            }

            model.materials[materialIndex] =
                    &importedMaterial;
        } catch (const std::exception &e) {
            model.materials[materialIndex] =
                    nullptr;

            LOG(std::format(
                "Material {} import failed: {}",
                materialIndex,
                e.what()
            ));
        }
    }
}

const Texture2D *ModelImporter::loadMaterialTexture(
    const aiMaterial *material,
    aiTextureType textureType,
    const aiScene *scene,
    const std::filesystem::path &modelPath
) {
    aiString textureReference;

    if (material->GetTexture(
            textureType,
            0,
            &textureReference
        ) != AI_SUCCESS) {
        return nullptr;
    }

    const auto [embeddedTexture, embeddedIndex] =
            scene->GetEmbeddedTextureAndIndex(
                textureReference.C_Str()
            );

    if (embeddedTexture) {
        const std::string embeddedId =
                embeddedIndex >= 0
                    ? std::to_string(embeddedIndex)
                    : std::string(
                        textureReference.C_Str()
                    );

        const std::string resourceKey =
                std::format(
                    "{}#embedded:{}",
                    modelPath.string(),
                    embeddedId
                );

        // compressed PNG/JPG data.
        //  mWidth = byte size.
        if (embeddedTexture->mHeight == 0) {
            const auto *bytes =
                    reinterpret_cast<const std::uint8_t *>(
                        embeddedTexture->pcData
                    );

            const std::span<const std::uint8_t>
                    encodedData{
                        bytes,
                        static_cast<std::size_t>(
                            embeddedTexture->mWidth
                        )
                    };

            return &resourceManager
                    .loadEncodedTexture(
                        resourceKey,
                        encodedData
                    );
        }

        const std::size_t pixelCount =
                static_cast<std::size_t>(
                    embeddedTexture->mWidth
                ) *
                static_cast<std::size_t>(
                    embeddedTexture->mHeight
                );

        std::vector<std::uint8_t> rgbaPixels(
            pixelCount * 4
        );

        for (std::size_t i = 0; i < pixelCount; ++i) {
            const aiTexel &texel = embeddedTexture->pcData[i];

            rgbaPixels[i * 4 + 0] = texel.r;
            rgbaPixels[i * 4 + 1] = texel.g;
            rgbaPixels[i * 4 + 2] = texel.b;
            rgbaPixels[i * 4 + 3] = texel.a;
        }

        return &resourceManager.loadRawTexture(
            resourceKey,
            static_cast<int>(
                embeddedTexture->mWidth
            ),
            static_cast<int>(
                embeddedTexture->mHeight
            ),
            rgbaPixels
        );
    }

    const std::filesystem::path texturePath =
    (
        modelPath.parent_path() /
        textureReference.C_Str()
    ).lexically_normal();

    return &resourceManager.loadTexture(
        texturePath.string()
    );
}
