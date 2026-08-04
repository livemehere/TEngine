#include "SandboxScene.h"

#include <array>

#include <camera/CameraComponent.h>
#include <graphics/CubeMap.h>
#include <rendering/Lights.h>
#include <rendering/mesh/MeshRendererComponent.h>
#include <rendering/mesh/InstancedMeshRendererComponent.h>
#include <rendering/mesh/materials/PhongMaterial.h>
#include <rendering/mesh/materials/UnlitMaterial.h>
#include <rendering/skybox/SkyboxComponent.h>
#include <resources/ResourceManager.h>
#include <scene/Scene.h>

void SandboxScene::build(Scene &scene, ResourceManager &resourceManager) {
    Entity &cameraEntity = scene.createEntity("Editor Camera");
    Transform &cameraTransform =
            cameraEntity.getComponent<TransformComponent>().local;
    cameraTransform.position = {9.0f, 6.0f, 11.0f};
    cameraTransform.lookAt({0.0f, 1.2f, 0.0f});
    cameraEntity.addComponent<CameraComponent>();
    scene.setActiveCamera(cameraEntity.id);

    const Mesh &planeMesh = resourceManager.getPlaneMesh();
    const Mesh &cubeMesh = resourceManager.getCubeMesh();

    const Texture2D &whiteTexture = resourceManager.getWhiteTexture();
    const Texture2D &boxTexture =
            resourceManager.loadTexture("textures/box.png");
    const Texture2D &boxSpecularMapTexture =
            resourceManager.loadTexture(
                "textures/box_specular_map.png",
                TextureColorSpace::Linear
            );
    const std::array<std::string, 6> skyboxFaces{
        "textures/skybox/right.jpg",
        "textures/skybox/left.jpg",
        "textures/skybox/top.jpg",
        "textures/skybox/bottom.jpg",
        "textures/skybox/front.jpg",
        "textures/skybox/back.jpg"
    };
    const CubeMap &skyboxCubeMap =
            resourceManager.loadCubeMap("defaultSkybox", skyboxFaces);

    const Shader &phongShader = resourceManager.getPhongShader();
    const Shader &unlitShader = resourceManager.getUnlitShader();

    Entity &skybox = scene.createEntity("Skybox");
    skybox.addComponent<SkyboxComponent>(&skyboxCubeMap);

    PhongMaterial &floorMaterial = resourceManager.loadPhongMaterial(
        "sandbox/floor",
        phongShader,
        whiteTexture
    );
    floorMaterial.baseColor = {0.42f, 0.46f, 0.52f, 1.0f};
    floorMaterial.specularTexture = &whiteTexture;
    floorMaterial.specularStrength = 0.12f;
    floorMaterial.shininess = 16.0f;
    floorMaterial.useBlinnPhong = true;

    PhongMaterial &boxMaterial = resourceManager.loadPhongMaterial(
        "sandbox/textured-box",
        phongShader,
        boxTexture
    );
    boxMaterial.baseColor = {0.75f, 0.75f, 0.75f, 1.0f};
    boxMaterial.specularTexture = &boxSpecularMapTexture;
    boxMaterial.specularStrength = 0.8f;
    boxMaterial.shininess = 64.0f;
    boxMaterial.useBlinnPhong = true;
    boxMaterial.environmentStrength = 0.08f;

    PhongMaterial &warmMaterial = resourceManager.loadPhongMaterial(
        "sandbox/warm",
        phongShader,
        whiteTexture
    );
    warmMaterial.baseColor = {0.72f, 0.28f, 0.12f, 1.0f};
    warmMaterial.specularTexture = &whiteTexture;
    warmMaterial.specularStrength = 0.65f;
    warmMaterial.shininess = 48.0f;
    warmMaterial.useBlinnPhong = true;

    PhongMaterial &coolMaterial = resourceManager.loadPhongMaterial(
        "sandbox/cool",
        phongShader,
        whiteTexture
    );
    coolMaterial.baseColor = {0.12f, 0.32f, 0.68f, 1.0f};
    coolMaterial.specularTexture = &whiteTexture;
    coolMaterial.specularStrength = 0.8f;
    coolMaterial.shininess = 72.0f;
    coolMaterial.useBlinnPhong = true;

    UnlitMaterial &pointLightMarkerMaterial =
            resourceManager.loadUnlitMaterial(
                "sandbox/point-light-marker",
                unlitShader,
                whiteTexture
            );
    pointLightMarkerMaterial.baseColor = {0.2f, 0.55f, 1.0f, 1.0f};

    UnlitMaterial &spotLightMarkerMaterial =
            resourceManager.loadUnlitMaterial(
                "sandbox/spot-light-marker",
                unlitShader,
                whiteTexture
            );
    spotLightMarkerMaterial.baseColor = {1.0f, 0.2f, 0.08f, 1.0f};

    const auto createCube = [&scene, &cubeMesh](
        const char *name,
        const Transform &transform,
        const Material &material
    ) -> Entity & {
        Entity &entity = scene.createEntity(name);
        entity.getComponent<TransformComponent>().local = transform;
        MeshRendererComponent &renderer =
                entity.addComponent<MeshRendererComponent>(
                    &cubeMesh,
                    &material
                );
        renderer.outlineMode = OutlineMode::ScaleFromPivot;
        return entity;
    };

    Entity &ground = scene.createEntity("Ground - Shadow Receiver");
    Transform &groundTransform =
            ground.getComponent<TransformComponent>().local;
    groundTransform.position = {0.0f, -0.05f, 0.0f};
    groundTransform.rotation = {-90.0f, 0.0f, 0.0f};
    groundTransform.scale = {16.0f, 16.0f, 1.0f};
    MeshRendererComponent &groundRenderer =
            ground.addComponent<MeshRendererComponent>(
                &planeMesh,
                &floorMaterial
            );
    groundRenderer.outlineMode = OutlineMode::ScaleFromPivot;

    Entity &backWall = scene.createEntity("Back Wall - Shadow Receiver");
    backWall.getComponent<TransformComponent>().local = {
        .position = {0.0f, 3.0f, -6.0f},
        .rotation = {0.0f, 0.0f, 0.0f},
        .scale = {16.0f, 6.0f, 1.0f}
    };
    MeshRendererComponent &backWallRenderer =
            backWall.addComponent<MeshRendererComponent>(
                &planeMesh,
                &floorMaterial
            );
    backWallRenderer.outlineMode = OutlineMode::ScaleFromPivot;

    (void)createCube(
        "Warm Tall Block",
        {
            .position = {-2.5f, 1.0f, -1.0f},
            .rotation = {0.0f, 18.0f, 0.0f},
            .scale = {1.25f, 2.0f, 1.25f}
        },
        warmMaterial
    );

    (void)createCube(
        "Textured Block",
        {
            .position = {1.8f, 0.75f, -0.8f},
            .rotation = {0.0f, -25.0f, 0.0f},
            .scale = {1.5f, 1.5f, 1.5f}
        },
        boxMaterial
    );

    (void)createCube(
        "Floating Cool Block",
        {
            .position = {0.2f, 2.8f, 1.5f},
            .rotation = {18.0f, 30.0f, 8.0f},
            .scale = {1.0f, 1.0f, 1.0f}
        },
        coolMaterial
    );

    (void)createCube(
        "Low Cool Block",
        {
            .position = {3.7f, 0.4f, 2.0f},
            .rotation = {0.0f, 35.0f, 0.0f},
            .scale = {1.4f, 0.8f, 1.4f}
        },
        coolMaterial
    );

    Entity &instancedCubes = scene.createEntity("Instanced Shadow Row");
    InstancedMeshRendererComponent &instancedRenderer =
            instancedCubes.addComponent<InstancedMeshRendererComponent>(
                &cubeMesh,
                &boxMaterial
            );

    for (int index = 0; index < 9; ++index) {
        Transform instanceTransform;
        instanceTransform.position = {
            -4.0f + static_cast<float>(index),
            0.25f + static_cast<float>(index % 3) * 0.15f,
            3.8f
        };
        instanceTransform.rotation.y = static_cast<float>(index) * 12.0f;
        instanceTransform.scale = glm::vec3(0.45f);
        (void)instancedRenderer.addInstance(instanceTransform);
    }

    const Model &bagModel =
            resourceManager.loadModel("models/backpack/backpack.obj", true);
    const EntityId bagId =
            scene.instantiateModel(bagModel, floorMaterial, "Backpack");
    if (Entity *bagEntity = scene.findEntity(bagId)) {
        bagEntity->getComponent<TransformComponent>().local = {
            .position = {-0.5f, 0.0f, -3.0f},
            .rotation = {0.0f, 25.0f, 0.0f},
            .scale = {0.55f, 0.55f, 0.55f}
        };
    }

    Entity &ambientLightEntity = scene.createEntity("Ambient Light");
    AmbientLightComponent &ambientLight =
            ambientLightEntity.addComponent<AmbientLightComponent>();
    ambientLight.color = {0.38f, 0.43f, 0.55f};
    ambientLight.intensity = 0.08f;

    Entity &directionalLightEntity = scene.createEntity("Warm Sun Light");
    directionalLightEntity.getComponent<TransformComponent>().local.rotation =
            {-48.0f, -32.0f, 0.0f};
    DirectionalLightComponent &directionalLight =
            directionalLightEntity.addComponent<DirectionalLightComponent>();
    directionalLight.color = {1.0f, 0.82f, 0.62f};
    directionalLight.intensity = 0.75f;
    directionalLight.castShadows = true;

    const glm::vec3 pointLightPosition{2.4f, 3.2f, 1.8f};
    Entity &pointLightEntity = scene.createEntity("Blue Point Light");
    pointLightEntity.getComponent<TransformComponent>().local.position =
            pointLightPosition;
    PointLightComponent &pointLight =
            pointLightEntity.addComponent<PointLightComponent>();
    pointLight.range = 9.0f;
    pointLight.color = {0.18f, 0.48f, 1.0f};
    pointLight.intensity = 5.0f;
    pointLight.castShadows = true;

    (void)createCube(
        "Blue Point Light Marker",
        {
            .position = pointLightPosition,
            .rotation = {0.0f, 0.0f, 0.0f},
            .scale = {0.18f, 0.18f, 0.18f}
        },
        pointLightMarkerMaterial
    );

    const glm::vec3 spotLightPosition{-3.8f, 5.0f, 3.0f};
    Entity &spotLightEntity = scene.createEntity("Red Spot Light");
    Transform &spotLightTransform =
            spotLightEntity.getComponent<TransformComponent>().local;
    spotLightTransform.position = spotLightPosition;
    spotLightTransform.lookAt({-2.0f, 0.0f, -0.5f});
    SpotLightComponent &spotLight =
            spotLightEntity.addComponent<SpotLightComponent>();
    spotLight.range = 12.0f;
    spotLight.color = {1.0f, 0.16f, 0.06f};
    spotLight.intensity = 4.0f;
    spotLight.innerAngle = 18.0f;
    spotLight.outerAngle = 30.0f;

    (void)createCube(
        "Red Spot Light Marker",
        {
            .position = spotLightPosition,
            .rotation = {0.0f, 0.0f, 0.0f},
            .scale = {0.16f, 0.16f, 0.16f}
        },
        spotLightMarkerMaterial
    );
}
