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
    cameraTransform.position = {17.0f, 9.0f, 19.0f};
    cameraTransform.lookAt({0.0f, 1.2f, -2.5f});
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
    const Texture2D &brickTexture =
            resourceManager.loadTexture("textures/brick/bricks2.jpg");
    const Texture2D &brickNormalTexture =
            resourceManager.loadTexture(
                "textures/brick/bricks2_normal.jpg",
                TextureColorSpace::Linear
            );
    const Texture2D &brickDepthTexture =
            resourceManager.loadTexture(
                "textures/brick/bricks2_disp.jpg",
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

    PhongMaterial &flatBrickMaterial = resourceManager.loadPhongMaterial(
        "sandbox/brick-flat",
        phongShader,
        brickTexture
    );
    flatBrickMaterial.baseColor = glm::vec4(1.0f);
    flatBrickMaterial.specularTexture = &whiteTexture;
    flatBrickMaterial.specularStrength = 0.2f;
    flatBrickMaterial.shininess = 24.0f;
    flatBrickMaterial.useBlinnPhong = true;

    PhongMaterial &normalMappedBrickMaterial =
            resourceManager.loadPhongMaterial(
                "sandbox/brick-normal-mapped",
                phongShader,
                brickTexture
            );
    normalMappedBrickMaterial.baseColor = glm::vec4(1.0f);
    normalMappedBrickMaterial.specularTexture = &whiteTexture;
    normalMappedBrickMaterial.normalTexture = &brickNormalTexture;
    normalMappedBrickMaterial.specularStrength = 0.2f;
    normalMappedBrickMaterial.shininess = 24.0f;
    normalMappedBrickMaterial.useBlinnPhong = true;
    normalMappedBrickMaterial.useNormalMapping = true;
    normalMappedBrickMaterial.normalStrength = 1.0f;
    // Texture2D flips image rows during loading, so this tutorial map needs
    // its tangent-space green channel restored.
    normalMappedBrickMaterial.flipNormalY = true;

    PhongMaterial &parallaxBrickMaterial =
            resourceManager.loadPhongMaterial(
                "sandbox/brick-parallax-occlusion",
                phongShader,
                brickTexture
            );
    parallaxBrickMaterial.baseColor = glm::vec4(1.0f);
    parallaxBrickMaterial.specularTexture = &whiteTexture;
    parallaxBrickMaterial.normalTexture = &brickNormalTexture;
    parallaxBrickMaterial.depthTexture = &brickDepthTexture;
    parallaxBrickMaterial.specularStrength = 0.2f;
    parallaxBrickMaterial.shininess = 24.0f;
    parallaxBrickMaterial.useBlinnPhong = true;
    parallaxBrickMaterial.useNormalMapping = true;
    parallaxBrickMaterial.normalStrength = 1.0f;
    parallaxBrickMaterial.flipNormalY = true;
    parallaxBrickMaterial.parallaxMappingMode =
            ParallaxMappingMode::Occlusion;
    parallaxBrickMaterial.parallaxScale = 0.08f;
    parallaxBrickMaterial.parallaxMinLayers = 8;
    parallaxBrickMaterial.parallaxMaxLayers = 32;
    parallaxBrickMaterial.discardParallaxEdges = false;

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

    PhongMaterial &warmMaterial = resourceManager.loadPhongMaterial(
        "sandbox/warm-clay",
        phongShader,
        whiteTexture
    );
    warmMaterial.baseColor = {0.68f, 0.22f, 0.12f, 1.0f};
    warmMaterial.specularTexture = &whiteTexture;
    warmMaterial.specularStrength = 0.18f;
    warmMaterial.shininess = 20.0f;
    warmMaterial.useBlinnPhong = true;

    PhongMaterial &mintMaterial = resourceManager.loadPhongMaterial(
        "sandbox/mint",
        phongShader,
        whiteTexture
    );
    mintMaterial.baseColor = {0.18f, 0.58f, 0.42f, 1.0f};
    mintMaterial.specularTexture = &whiteTexture;
    mintMaterial.specularStrength = 0.25f;
    mintMaterial.shininess = 32.0f;
    mintMaterial.useBlinnPhong = true;

    UnlitMaterial &pointLightMarkerMaterial =
            resourceManager.loadUnlitMaterial(
                "sandbox/point-light-marker",
                unlitShader,
                whiteTexture
            );
    pointLightMarkerMaterial.baseColor = {8.0f, 7.2f, 5.5f, 1.0f};

    UnlitMaterial &spotLightMarkerMaterial =
            resourceManager.loadUnlitMaterial(
                "sandbox/spot-light-marker",
                unlitShader,
                whiteTexture
            );
    spotLightMarkerMaterial.baseColor = {5.0f, 0.3f, 0.1f, 1.0f};

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

    const auto createPlane = [&scene, &planeMesh](
        const char *name,
        const Transform &transform,
        const Material &material
    ) -> Entity & {
        Entity &entity = scene.createEntity(name);
        entity.getComponent<TransformComponent>().local = transform;
        MeshRendererComponent &renderer =
                entity.addComponent<MeshRendererComponent>(
                    &planeMesh,
                    &material
                );
        renderer.outlineMode = OutlineMode::ScaleFromPivot;
        return entity;
    };

    (void)createPlane(
        "Ground - SSAO Receiver",
        {
            .position = {0.0f, -0.05f, -1.0f},
            .rotation = {-90.0f, 0.0f, 0.0f},
            .scale = {30.0f, 24.0f, 1.0f}
        },
        floorMaterial
    );
    (void)createPlane(
        "Back Wall - SSAO Corner",
        {
            .position = {0.0f, 4.0f, -11.0f},
            .rotation = {0.0f, 0.0f, 0.0f},
            .scale = {30.0f, 8.0f, 1.0f}
        },
        floorMaterial
    );
    (void)createPlane(
        "Left Wall - SSAO Corner",
        {
            .position = {-14.0f, 4.0f, -1.0f},
            .rotation = {0.0f, 90.0f, 0.0f},
            .scale = {20.0f, 8.0f, 1.0f}
        },
        floorMaterial
    );

    // Three adjacent material samples make the contact darkening along the
    // floor easy to compare without changing geometry.
    (void)createCube(
        "Brick Sample - Flat",
        {
            .position = {-3.0f, 0.9f, 2.2f},
            .rotation = {0.0f, -12.0f, 0.0f},
            .scale = {1.7f, 1.7f, 1.7f}
        },
        flatBrickMaterial
    );
    (void)createCube(
        "Brick Sample - Normal Mapped",
        {
            .position = {-0.5f, 0.9f, 2.0f},
            .rotation = {0.0f, -12.0f, 0.0f},
            .scale = {1.7f, 1.7f, 1.7f}
        },
        normalMappedBrickMaterial
    );
    (void)createCube(
        "Brick Sample - Parallax Occlusion",
        {
            .position = {2.0f, 0.9f, 1.8f},
            .rotation = {0.0f, -12.0f, 0.0f},
            .scale = {1.7f, 1.7f, 1.7f}
        },
        parallaxBrickMaterial
    );

    // The narrow gaps under and between these pieces are deliberately sized
    // to reveal the SSAO radius and bias controls.
    (void)createCube(
        "Arch - Left Pillar",
        {
            .position = {3.8f, 1.5f, -4.8f},
            .rotation = {0.0f, 0.0f, 0.0f},
            .scale = {0.9f, 3.0f, 0.9f}
        },
        warmMaterial
    );
    (void)createCube(
        "Arch - Right Pillar",
        {
            .position = {6.4f, 1.5f, -4.8f},
            .rotation = {0.0f, 0.0f, 0.0f},
            .scale = {0.9f, 3.0f, 0.9f}
        },
        warmMaterial
    );
    (void)createCube(
        "Arch - Top Beam",
        {
            .position = {5.1f, 3.35f, -4.8f},
            .rotation = {0.0f, 0.0f, 0.0f},
            .scale = {3.5f, 0.7f, 0.9f}
        },
        mintMaterial
    );
    (void)createCube(
        "Arch - Suspended Block",
        {
            .position = {5.1f, 1.45f, -4.8f},
            .rotation = {12.0f, 28.0f, 5.0f},
            .scale = {0.9f, 0.9f, 0.9f}
        },
        coolMaterial
    );

    (void)createCube(
        "Backpack Display Plinth",
        {
            .position = {-6.4f, 0.3f, -4.8f},
            .rotation = {0.0f, 8.0f, 0.0f},
            .scale = {3.0f, 0.6f, 3.0f}
        },
        mintMaterial
    );

    const Model &bagModel =
            resourceManager.loadModel("models/backpack/backpack.obj", true);
    const EntityId bagId =
            scene.instantiateModel(bagModel, floorMaterial, "Upright Backpack");
    if (Entity *bagEntity = scene.findEntity(bagId)) {
        bagEntity->getComponent<TransformComponent>().local = {
            .position = {-6.4f, 0.62f, -4.8f},
            .rotation = {0.0f, -28.0f, 0.0f},
            .scale = {0.9f, 0.9f, 0.9f}
        };
    }

    (void)createCube(
        "Wide Step - Lower",
        {
            .position = {8.8f, 0.25f, 1.6f},
            .rotation = {0.0f, -8.0f, 0.0f},
            .scale = {3.8f, 0.5f, 3.2f}
        },
        coolMaterial
    );
    (void)createCube(
        "Wide Step - Upper",
        {
            .position = {8.7f, 0.85f, 0.9f},
            .rotation = {0.0f, -8.0f, 0.0f},
            .scale = {2.7f, 0.7f, 2.2f}
        },
        mintMaterial
    );
    (void)createCube(
        "Environment Mapping Sample",
        {
            .position = {8.5f, 2.1f, 0.3f},
            .rotation = {15.0f, 30.0f, 8.0f},
            .scale = {1.3f, 1.3f, 1.3f}
        },
        boxMaterial
    );

    Entity &instancedCubes = scene.createEntity("Instanced Occlusion Row");
    InstancedMeshRendererComponent &instancedRenderer =
            instancedCubes.addComponent<InstancedMeshRendererComponent>(
                &cubeMesh,
                &warmMaterial
            );
    for (int index = 0; index < 11; ++index) {
        Transform instanceTransform;
        instanceTransform.position = {
            -8.5f + static_cast<float>(index) * 1.6f,
            0.35f + static_cast<float>(index % 3) * 0.12f,
            -8.6f
        };
        instanceTransform.rotation.y = static_cast<float>(index) * 9.0f;
        instanceTransform.scale = {0.65f, 0.7f, 0.65f};
        (void)instancedRenderer.addInstance(instanceTransform);
    }

    Entity &ambientLightEntity = scene.createEntity("Ambient Fill");
    AmbientLightComponent &ambientLight =
            ambientLightEntity.addComponent<AmbientLightComponent>();
    ambientLight.color = {0.42f, 0.46f, 0.58f};
    ambientLight.intensity = 0.18f;

    Entity &directionalLightEntity = scene.createEntity("Warm Sun Light");
    directionalLightEntity.getComponent<TransformComponent>().local.rotation =
            {-52.0f, -28.0f, 0.0f};
    DirectionalLightComponent &directionalLight =
            directionalLightEntity.addComponent<DirectionalLightComponent>();
    directionalLight.color = {1.0f, 0.84f, 0.68f};
    directionalLight.intensity = 0.48f;
    directionalLight.castShadows = true;

    const glm::vec3 pointLightPosition{1.5f, 6.5f, 6.5f};
    Entity &pointLightEntity = scene.createEntity("HDR Key Light");
    pointLightEntity.getComponent<TransformComponent>().local.position =
            pointLightPosition;
    PointLightComponent &pointLight =
            pointLightEntity.addComponent<PointLightComponent>();
    pointLight.range = 18.0f;
    pointLight.color = {1.0f, 0.94f, 0.82f};
    pointLight.intensity = 11.0f;
    pointLight.castShadows = true;

    (void)createCube(
        "HDR Key Light Marker",
        {
            .position = pointLightPosition,
            .rotation = {0.0f, 0.0f, 0.0f},
            .scale = {0.22f, 0.22f, 0.22f}
        },
        pointLightMarkerMaterial
    );

    const glm::vec3 spotLightPosition{-9.0f, 6.5f, 1.5f};
    Entity &spotLightEntity = scene.createEntity("Red Gallery Spot");
    Transform &spotLightTransform =
            spotLightEntity.getComponent<TransformComponent>().local;
    spotLightTransform.position = spotLightPosition;
    spotLightTransform.lookAt({-6.0f, 1.0f, -5.0f});
    SpotLightComponent &spotLight =
            spotLightEntity.addComponent<SpotLightComponent>();
    spotLight.range = 16.0f;
    spotLight.color = {1.0f, 0.18f, 0.08f};
    spotLight.intensity = 3.0f;
    spotLight.innerAngle = 16.0f;
    spotLight.outerAngle = 28.0f;

    (void)createCube(
        "Red Gallery Spot Marker",
        {
            .position = spotLightPosition,
            .rotation = {0.0f, 0.0f, 0.0f},
            .scale = {0.18f, 0.18f, 0.18f}
        },
        spotLightMarkerMaterial
    );
}
