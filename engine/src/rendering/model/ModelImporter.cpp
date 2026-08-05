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
#include "../mesh/materials/PhongMaterial.h"
#include "../mesh/materials/PBRMaterial.h"

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
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace;

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

        glm::vec4 tangent{
            0.0f,
            0.0f,
            0.0f,
            1.0f
        };

        if (mesh->HasTangentsAndBitangents()) {
            const aiVector3D &sourceTangent =
                    mesh->mTangents[i];
            const aiVector3D &sourceBitangent =
                    mesh->mBitangents[i];
            const glm::vec3 tangentDirection{
                sourceTangent.x,
                sourceTangent.y,
                sourceTangent.z
            };
            const glm::vec3 bitangentDirection{
                sourceBitangent.x,
                sourceBitangent.y,
                sourceBitangent.z
            };
            tangent = glm::vec4(
                tangentDirection,
                glm::dot(
                    glm::cross(normal, tangentDirection),
                    bitangentDirection
                ) < 0.0f
                    ? -1.0f
                    : 1.0f
            );
        }

        vertices.emplace_back(
            position,
            normal,
            texCoord,
            tangent
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
            float metallicFactor = 0.0f;
            float roughnessFactor = 0.5f;
            const bool hasMetallicFactor = sourceMaterial->Get(
                AI_MATKEY_METALLIC_FACTOR,
                metallicFactor
            ) == AI_SUCCESS;
            const bool hasRoughnessFactor = sourceMaterial->Get(
                AI_MATKEY_ROUGHNESS_FACTOR,
                roughnessFactor
            ) == AI_SUCCESS;
            const bool hasPBRTextures =
                    sourceMaterial->GetTextureCount(
                        aiTextureType_BASE_COLOR
                    ) > 0 ||
                    sourceMaterial->GetTextureCount(
                        aiTextureType_METALNESS
                    ) > 0 ||
                    sourceMaterial->GetTextureCount(
                        aiTextureType_DIFFUSE_ROUGHNESS
                    ) > 0;
            const bool usesPBR = hasMetallicFactor ||
                                 hasRoughnessFactor ||
                                 hasPBRTextures;

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

            if (!albedoTexture && usesPBR) {
                albedoTexture = &resourceManager.getWhiteTexture();
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

            int twoSided = 0;
            const bool isTwoSided = sourceMaterial->Get(
                AI_MATKEY_TWOSIDED,
                twoSided
            ) == AI_SUCCESS && twoSided != 0;

            if (usesPBR) {
                PBRMaterial &importedMaterial =
                        resourceManager.loadPBRMaterial(
                            materialKey,
                            resourceManager.getPBRShader(),
                            *albedoTexture
                        );
                importedMaterial.baseColor = {
                    baseColor.r,
                    baseColor.g,
                    baseColor.b,
                    baseColor.a
                };
                importedMaterial.metallic = hasMetallicFactor
                                                ? metallicFactor
                                                : 1.0f;
                importedMaterial.roughness = hasRoughnessFactor
                                                 ? roughnessFactor
                                                 : 1.0f;
                importedMaterial.ao = 1.0f;
                importedMaterial.rasterState.cullMode = isTwoSided
                                                            ? CullMode::None
                                                            : CullMode::Back;
                importedMaterial.normalTexture = loadMaterialTexture(
                    sourceMaterial,
                    aiTextureType_NORMALS,
                    scene,
                    modelPath
                );
                if (!importedMaterial.normalTexture) {
                    importedMaterial.normalTexture = loadMaterialTexture(
                        sourceMaterial,
                        aiTextureType_HEIGHT,
                        scene,
                        modelPath
                    );
                }
                importedMaterial.metallicTexture = loadMaterialTexture(
                    sourceMaterial,
                    aiTextureType_METALNESS,
                    scene,
                    modelPath
                );
                importedMaterial.roughnessTexture = loadMaterialTexture(
                    sourceMaterial,
                    aiTextureType_DIFFUSE_ROUGHNESS,
                    scene,
                    modelPath
                );
                importedMaterial.aoTexture = loadMaterialTexture(
                    sourceMaterial,
                    aiTextureType_AMBIENT_OCCLUSION,
                    scene,
                    modelPath
                );

                if (importedMaterial.metallicTexture &&
                    importedMaterial.metallicTexture ==
                        importedMaterial.roughnessTexture) {
                    importedMaterial.metallicChannel = 2;
                    importedMaterial.roughnessChannel = 1;
                } else {
                    importedMaterial.metallicChannel = 0;
                    importedMaterial.roughnessChannel = 0;
                }
                importedMaterial.aoChannel = 0;
                model.materials[materialIndex] = &importedMaterial;
                continue;
            }

            PhongMaterial &importedMaterial =
                    resourceManager.loadPhongMaterial(
                        materialKey,
                        resourceManager.getPhongShader(),
                        *albedoTexture
                    );

            importedMaterial.baseColor = {
                baseColor.r,
                baseColor.g,
                baseColor.b,
                baseColor.a
            };

            if (isTwoSided) {
                importedMaterial.rasterState.cullMode = CullMode::None;
            }

            importedMaterial.specularTexture =
                    nullptr;
            importedMaterial.normalTexture =
                    nullptr;
            importedMaterial.depthTexture =
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

            try {
                importedMaterial.normalTexture =
                        loadMaterialTexture(
                            sourceMaterial,
                            aiTextureType_NORMALS,
                            scene,
                            modelPath
                        );
                if (!importedMaterial.normalTexture) {
                    importedMaterial.normalTexture =
                            loadMaterialTexture(
                                sourceMaterial,
                                aiTextureType_HEIGHT,
                                scene,
                                modelPath
                            );
                }
            } catch (const std::exception &e) {
                LOG(std::format(
                    "Normal texture load failed: {}",
                    e.what()
                ));
            }

            try {
                importedMaterial.depthTexture =
                        loadMaterialTexture(
                            sourceMaterial,
                            aiTextureType_DISPLACEMENT,
                            scene,
                            modelPath
                        );
            } catch (const std::exception &e) {
                LOG(std::format(
                    "Displacement texture load failed: {}",
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
    const TextureColorSpace colorSpace =
            textureType == aiTextureType_BASE_COLOR ||
            textureType == aiTextureType_DIFFUSE
                ? TextureColorSpace::SRGB
                : TextureColorSpace::Linear;

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
                        encodedData,
                        colorSpace
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
            rgbaPixels,
            colorSpace
        );
    }

    const std::filesystem::path texturePath =
    (
        modelPath.parent_path() /
        textureReference.C_Str()
    ).lexically_normal();

    return &resourceManager.loadTexture(
        texturePath.string(),
        colorSpace
    );
}
