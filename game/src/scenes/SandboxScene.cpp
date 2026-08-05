#include "SandboxScene.h"

#include <array>

#include "components/MotionComponents.h"

#include <camera/CameraComponent.h>
#include <graphics/CubeMap.h>
#include <rendering/Lights.h>
#include <rendering/mesh/MeshRendererComponent.h>
#include <rendering/mesh/InstancedMeshRendererComponent.h>
#include <rendering/mesh/materials/PhongMaterial.h>
#include <rendering/mesh/materials/PBRMaterial.h>
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
    const Mesh &sphereMesh = resourceManager.getSphereMesh();

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
    const Texture2D &rustyMetalAlbedo = resourceManager.loadTexture(
        "textures/pbr/rusty_metal_04/albedo.jpg"
    );
    const Texture2D &rustyMetalNormal = resourceManager.loadTexture(
        "textures/pbr/rusty_metal_04/normal_gl.jpg",
        TextureColorSpace::Linear
    );
    const Texture2D &rustyMetalMetallic = resourceManager.loadTexture(
        "textures/pbr/rusty_metal_04/metallic.jpg",
        TextureColorSpace::Linear
    );
    const Texture2D &rustyMetalRoughness = resourceManager.loadTexture(
        "textures/pbr/rusty_metal_04/roughness.jpg",
        TextureColorSpace::Linear
    );
    const Texture2D &rustyMetalAO = resourceManager.loadTexture(
        "textures/pbr/rusty_metal_04/ao.jpg",
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
    const Shader &pbrShader = resourceManager.getPBRShader();
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

    const auto makePBRMaterial = [&](
        const std::string &name,
        const glm::vec4 &color,
        const float metallic,
        const float roughness
    ) -> PBRMaterial & {
        PBRMaterial &material = resourceManager.loadPBRMaterial(
            "sandbox/" + name,
            pbrShader,
            whiteTexture
        );
        material.baseColor = color;
        material.metallic = metallic;
        material.roughness = roughness;
        material.ao = 1.0f;
        return material;
    };

    PBRMaterial &pbrDielectricSmooth = makePBRMaterial(
        "pbr-dielectric-smooth",
        {0.72f, 0.06f, 0.04f, 1.0f},
        0.0f,
        0.15f
    );
    PBRMaterial &pbrDielectricRough = makePBRMaterial(
        "pbr-dielectric-rough",
        {0.72f, 0.06f, 0.04f, 1.0f},
        0.0f,
        0.8f
    );
    PBRMaterial &pbrMetalSmooth = makePBRMaterial(
        "pbr-metal-smooth",
        {1.0f, 0.71f, 0.22f, 1.0f},
        1.0f,
        0.15f
    );
    PBRMaterial &pbrMetalRough = makePBRMaterial(
        "pbr-metal-rough",
        {1.0f, 0.71f, 0.22f, 1.0f},
        1.0f,
        0.72f
    );
    PBRMaterial &rustyMetalMaterial = resourceManager.loadPBRMaterial(
        "sandbox/pbr-rusty-metal-04",
        pbrShader,
        rustyMetalAlbedo
    );
    rustyMetalMaterial.normalTexture = &rustyMetalNormal;
    rustyMetalMaterial.metallicTexture = &rustyMetalMetallic;
    rustyMetalMaterial.roughnessTexture = &rustyMetalRoughness;
    rustyMetalMaterial.aoTexture = &rustyMetalAO;
    rustyMetalMaterial.metallic = 1.0f;
    rustyMetalMaterial.roughness = 1.0f;
    rustyMetalMaterial.ao = 1.0f;
    rustyMetalMaterial.flipNormalY = true;

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

    const auto createSphere = [&scene, &sphereMesh](
        const char *name,
        const Transform &transform,
        const Material &material
    ) -> Entity & {
        Entity &entity = scene.createEntity(name);
        entity.getComponent<TransformComponent>().local = transform;
        entity.addComponent<MeshRendererComponent>(
            &sphereMesh,
            &material
        );
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

    const std::array<std::pair<const char *, PBRMaterial *>, 5>
            pbrSamples{{
                {"PBR - Dielectric Smooth", &pbrDielectricSmooth},
                {"PBR - Dielectric Rough", &pbrDielectricRough},
                {"PBR - Metal Smooth", &pbrMetalSmooth},
                {"PBR - Metal Rough", &pbrMetalRough},
                {"PBR - Textured Rusty Metal", &rustyMetalMaterial}
            }};
    for (std::size_t index = 0; index < pbrSamples.size(); ++index) {
        Entity &sample = createSphere(
            pbrSamples[index].first,
            {
                .position = {
                    -6.0f + static_cast<float>(index) * 3.0f,
                    1.05f,
                    5.2f
                },
                .rotation = {0.0f, 0.0f, 0.0f},
                .scale = {2.0f, 2.0f, 2.0f}
            },
            *pbrSamples[index].second
        );
        if (index == pbrSamples.size() - 1) {
            SpinComponent &spin = sample.addComponent<SpinComponent>();
            spin.degreesPerSecond = {0.0f, 18.0f, 0.0f};
        }
    }

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
    Entity &suspendedBlock = createCube(
        "Arch - Suspended Block",
        {
            .position = {5.1f, 1.45f, -4.8f},
            .rotation = {12.0f, 28.0f, 5.0f},
            .scale = {0.9f, 0.9f, 0.9f}
        },
        coolMaterial
    );
    SpinComponent &suspendedSpin =
            suspendedBlock.addComponent<SpinComponent>();
    suspendedSpin.degreesPerSecond = {18.0f, 55.0f, 12.0f};
    BobComponent &suspendedBob =
            suspendedBlock.addComponent<BobComponent>();
    suspendedBob.amplitude = 0.65f;
    suspendedBob.frequency = 0.42f;

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
        SpinComponent &displaySpin =
                bagEntity->addComponent<SpinComponent>();
        displaySpin.degreesPerSecond = {0.0f, 16.0f, 0.0f};
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
    Entity &environmentSample = createCube(
        "Environment Mapping Sample",
        {
            .position = {8.5f, 2.1f, 0.3f},
            .rotation = {15.0f, 30.0f, 8.0f},
            .scale = {1.3f, 1.3f, 1.3f}
        },
        boxMaterial
    );
    SpinComponent &environmentSpin =
            environmentSample.addComponent<SpinComponent>();
    environmentSpin.degreesPerSecond = {12.0f, 32.0f, 6.0f};

    Entity &orbitingCube = createCube(
        "Orbiting Gallery Cube",
        {
            .position = {1.6f, 2.8f, -4.8f},
            .rotation = {18.0f, 0.0f, 18.0f},
            .scale = {0.75f, 0.75f, 0.75f}
        },
        coolMaterial
    );
    OrbitComponent &orbit = orbitingCube.addComponent<OrbitComponent>();
    orbit.center = {5.1f, 2.8f, -4.8f};
    orbit.degreesPerSecond = 24.0f;
    SpinComponent &orbitingSpin = orbitingCube.addComponent<SpinComponent>();
    orbitingSpin.degreesPerSecond = {35.0f, 60.0f, 20.0f};

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

    // Localized gallery lights make the roughness-dependent PBR highlight
    // readable without flooding the older Phong samples behind the row.
    const std::array<glm::vec3, 2> pbrLightPositions{
        glm::vec3{-3.5f, 4.5f, 10.0f},
        glm::vec3{4.0f, 4.5f, 10.0f}
    };
    for (std::size_t index = 0; index < pbrLightPositions.size(); ++index) {
        Entity &lightEntity = scene.createEntity(
            index == 0 ? "PBR Gallery Light - Left"
                       : "PBR Gallery Light - Right"
        );
        lightEntity.getComponent<TransformComponent>().local.position =
                pbrLightPositions[index];
        PointLightComponent &galleryLight =
                lightEntity.addComponent<PointLightComponent>();
        galleryLight.range = 8.0f;
        galleryLight.color = index == 0
                                 ? glm::vec3{1.0f, 0.72f, 0.48f}
                                 : glm::vec3{0.62f, 0.78f, 1.0f};
        galleryLight.intensity = 2.5f;
        galleryLight.castShadows = false;
    }

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
