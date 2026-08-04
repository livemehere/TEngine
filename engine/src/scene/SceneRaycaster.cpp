#include "SceneRaycaster.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "Scene.h"
#include "TransformComponent.h"
#include "../rendering/mesh/InstancedMeshRendererComponent.h"
#include "../rendering/mesh/MeshRendererComponent.h"
#include "../rendering/model/InstancedModelRendererComponent.h"

namespace {
    constexpr float Epsilon = 1e-6f;

    bool intersectsBounds(
        const Ray &ray,
        const MeshBounds &bounds,
        float maximumDistance
    ) {
        float nearDistance = 0.0f;
        float farDistance = maximumDistance;

        for (int axis = 0; axis < 3; ++axis) {
            const float origin = ray.origin[axis];
            const float direction = ray.direction[axis];
            if (std::abs(direction) <= Epsilon) {
                if (origin < bounds.min[axis] || origin > bounds.max[axis]) {
                    return false;
                }
                continue;
            }

            float first = (bounds.min[axis] - origin) / direction;
            float second = (bounds.max[axis] - origin) / direction;
            if (first > second) {
                std::swap(first, second);
            }

            nearDistance = std::max(nearDistance, first);
            farDistance = std::min(farDistance, second);
            if (nearDistance > farDistance) {
                return false;
            }
        }

        return farDistance >= 0.0f;
    }

    std::optional<float> intersectTriangle(
        const Ray &ray,
        const glm::vec3 &first,
        const glm::vec3 &second,
        const glm::vec3 &third
    ) {
        const glm::vec3 edge1 = second - first;
        const glm::vec3 edge2 = third - first;
        const glm::vec3 p = glm::cross(ray.direction, edge2);
        const float determinant = glm::dot(edge1, p);
        if (std::abs(determinant) <= Epsilon) {
            return std::nullopt;
        }

        const float inverseDeterminant = 1.0f / determinant;
        const glm::vec3 offset = ray.origin - first;
        const float u = glm::dot(offset, p) * inverseDeterminant;
        if (u < 0.0f || u > 1.0f) {
            return std::nullopt;
        }

        const glm::vec3 q = glm::cross(offset, edge1);
        const float v = glm::dot(ray.direction, q) * inverseDeterminant;
        if (v < 0.0f || u + v > 1.0f) {
            return std::nullopt;
        }

        const float distance = glm::dot(edge2, q) * inverseDeterminant;
        return distance >= 0.0f
                   ? std::optional<float>(distance)
                   : std::nullopt;
    }

    std::optional<float> intersectMesh(
        const Ray &worldRay,
        const Mesh &mesh,
        const glm::mat4 &worldMatrix,
        float maximumDistance
    ) {
        if (std::abs(glm::determinant(glm::mat3(worldMatrix))) <= Epsilon) {
            return std::nullopt;
        }

        const glm::mat4 inverseWorld = glm::inverse(worldMatrix);
        const Ray localRay{
            .origin = glm::vec3(inverseWorld * glm::vec4(worldRay.origin, 1.0f)),
            .direction = glm::vec3(inverseWorld * glm::vec4(worldRay.direction, 0.0f))
        };

        if (!intersectsBounds(localRay, mesh.getBounds(), maximumDistance)) {
            return std::nullopt;
        }

        const std::vector<glm::vec3> &positions = mesh.getPositions();
        const std::vector<GLuint> &indices = mesh.getIndices();
        float closest = maximumDistance;
        bool found = false;

        for (std::size_t index = 0; index + 2 < indices.size(); index += 3) {
            const GLuint first = indices[index];
            const GLuint second = indices[index + 1];
            const GLuint third = indices[index + 2];
            if (first >= positions.size() ||
                second >= positions.size() ||
                third >= positions.size()) {
                continue;
            }

            const std::optional<float> distance = intersectTriangle(
                localRay,
                positions[first],
                positions[second],
                positions[third]
            );
            if (distance && *distance < closest) {
                closest = *distance;
                found = true;
            }
        }

        return found ? std::optional<float>(closest) : std::nullopt;
    }

    void considerHit(
        std::optional<SceneRaycastHit> &closestHit,
        const Ray &ray,
        const Mesh &mesh,
        const glm::mat4 &worldMatrix,
        EntityId entityId,
        SceneRaycastTarget target,
        std::optional<InstanceId> instanceId
    ) {
        const float maximumDistance = closestHit
                                          ? closestHit->distance
                                          : std::numeric_limits<float>::max();
        const std::optional<float> distance = intersectMesh(
            ray,
            mesh,
            worldMatrix,
            maximumDistance
        );
        if (!distance) {
            return;
        }

        closestHit = SceneRaycastHit{
            .entityId = entityId,
            .target = target,
            .instanceId = instanceId,
            .distance = *distance
        };
    }
}

std::optional<SceneRaycastHit> SceneRaycaster::cast(
    const Scene &scene,
    const Ray &ray
) {
    std::optional<SceneRaycastHit> closestHit;

    scene.each<TransformComponent, MeshRendererComponent>(
        [&](const Entity &entity,
            const TransformComponent &,
            const MeshRendererComponent &component) {
            if (!component.enabled || !component.mesh || !component.material) {
                return;
            }
            considerHit(
                closestHit,
                ray,
                *component.mesh,
                scene.getWorldMatrix(entity),
                entity.id,
                SceneRaycastTarget::Entity,
                std::nullopt
            );
        }
    );

    scene.each<TransformComponent, InstancedMeshRendererComponent>(
        [&](const Entity &entity,
            const TransformComponent &,
            const InstancedMeshRendererComponent &component) {
            if (!component.enabled ||
                !component.mesh ||
                !component.material ||
                component.material->renderQueue != RenderQueueType::Opaque) {
                return;
            }
            const glm::mat4 entityWorld = scene.getWorldMatrix(entity);
            for (const InstanceData &instance : component.instances.getItems()) {
                considerHit(
                    closestHit,
                    ray,
                    *component.mesh,
                    entityWorld * instance.transform.getLocalMatrix(),
                    entity.id,
                    SceneRaycastTarget::MeshInstance,
                    instance.id
                );
            }
        }
    );

    scene.each<TransformComponent, InstancedModelRendererComponent>(
        [&](const Entity &entity,
            const TransformComponent &,
            const InstancedModelRendererComponent &component) {
            if (!component.enabled || !component.model) {
                return;
            }

            const Model &model = *component.model;
            const glm::mat4 entityWorld = scene.getWorldMatrix(entity);
            std::vector<glm::mat4> nodeMatrices(model.nodes.size(), glm::mat4{1.0f});

            for (std::size_t nodeIndex = 0; nodeIndex < model.nodes.size(); ++nodeIndex) {
                const ModelNode &node = model.nodes[nodeIndex];
                const glm::mat4 nodeLocal = node.localTransform.getLocalMatrix();
                nodeMatrices[nodeIndex] = node.parentIndex
                                              ? nodeMatrices[*node.parentIndex] * nodeLocal
                                              : nodeLocal;

                for (const std::size_t partIndex : node.partIndices) {
                    if (partIndex >= model.parts.size() ||
                        !model.parts[partIndex].mesh) {
                        continue;
                    }
                    const ModelPart &part = model.parts[partIndex];
                    const Material *material = component.fallbackMaterial;
                    if (part.materialSlot < model.materials.size() &&
                        model.materials[part.materialSlot]) {
                        material = model.materials[part.materialSlot];
                    }
                    if (!material || material->renderQueue != RenderQueueType::Opaque) {
                        continue;
                    }

                    const Mesh &mesh = *part.mesh;
                    for (const InstanceData &instance : component.instances.getItems()) {
                        considerHit(
                            closestHit,
                            ray,
                            mesh,
                            entityWorld *
                            instance.transform.getLocalMatrix() *
                            nodeMatrices[nodeIndex],
                            entity.id,
                            SceneRaycastTarget::ModelInstance,
                            instance.id
                        );
                    }
                }
            }
        }
    );

    return closestHit;
}
