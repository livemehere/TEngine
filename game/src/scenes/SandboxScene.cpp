#include "SandboxScene.h"

#include <array>
#include <string>

#include "components/MotionComponents.h"

#include <camera/CameraComponent.h>
#include <rendering/Lights.h>
#include <rendering/mesh/MeshRendererComponent.h>
#include <rendering/mesh/materials/PBRMaterial.h>
#include <resources/ResourceManager.h>
#include <scene/Scene.h>

namespace {
struct ShowcaseMaterial {
    const char *name;
    const char *albedoPath;
    const char *normalPath;
    const char *roughnessPath;
    const char *aoPath = nullptr;
    bool metallic = false;
    float normalStrength = 1.0f;
};

constexpr std::array<ShowcaseMaterial, 9> ShowcaseMaterials{{
    {
        .name = "Blue Metal Plate",
        .albedoPath =
            "textures/pbr/blue_metal_plate/blue_metal_plate_diff_1k.jpg",
        .normalPath =
            "textures/pbr/blue_metal_plate/blue_metal_plate_nor_gl_1k.png",
        .roughnessPath =
            "textures/pbr/blue_metal_plate/blue_metal_plate_rough_1k.png",
        .normalStrength = 1.15f
    },
    {
        .name = "Broken Brick Wall",
        .albedoPath =
            "textures/pbr/broken_brick_wall/broken_brick_wall_diff_1k.jpg",
        .normalPath =
            "textures/pbr/broken_brick_wall/broken_brick_wall_nor_gl_1k.png",
        .roughnessPath =
            "textures/pbr/broken_brick_wall/broken_brick_wall_rough_1k.png",
        .normalStrength = 1.2f
    },
    {
        .name = "Clay Roof Tiles",
        .albedoPath =
            "textures/pbr/clay_roof_tiles_03/clay_roof_tiles_03_diff_1k.jpg",
        .normalPath =
            "textures/pbr/clay_roof_tiles_03/clay_roof_tiles_03_nor_gl_1k.png",
        .roughnessPath =
            "textures/pbr/clay_roof_tiles_03/clay_roof_tiles_03_rough_1k.png",
        .normalStrength = 1.1f
    },
    {
        .name = "Clean Asphalt",
        .albedoPath =
            "textures/pbr/clean_asphalt/clean_asphalt_diff_1k.jpg",
        .normalPath =
            "textures/pbr/clean_asphalt/clean_asphalt_nor_gl_1k.png",
        .roughnessPath =
            "textures/pbr/clean_asphalt/clean_asphalt_rough_1k.png",
        .normalStrength = 1.35f
    },
    {
        .name = "Dark Rock",
        .albedoPath =
            "textures/pbr/dark_rock_02/dark_rock_02_diff_1k.jpg",
        .normalPath =
            "textures/pbr/dark_rock_02/dark_rock_02_nor_gl_1k.png",
        .roughnessPath =
            "textures/pbr/dark_rock_02/dark_rock_02_rough_1k.png",
        .normalStrength = 1.2f
    },
    {
        .name = "Flour",
        .albedoPath = "textures/pbr/flour/flour_diff_1k.jpg",
        .normalPath = "textures/pbr/flour/flour_nor_gl_1k.png",
        .roughnessPath = "textures/pbr/flour/flour_rough_1k.png",
        .aoPath = "textures/pbr/flour/flour_ao_1k.jpg",
        .normalStrength = 0.65f
    },
    {
        .name = "Forest Leaves",
        .albedoPath =
            "textures/pbr/forest_leaves_02/forest_leaves_02_diffuse_1k.jpg",
        .normalPath =
            "textures/pbr/forest_leaves_02/forest_leaves_02_nor_gl_1k.png",
        .roughnessPath =
            "textures/pbr/forest_leaves_02/forest_leaves_02_rough_1k.jpg",
        .normalStrength = 1.1f
    },
    {
        .name = "Damaged Road",
        .albedoPath =
            "textures/pbr/road_damaged_2/road_damaged_2_diff_1k.jpg",
        .normalPath =
            "textures/pbr/road_damaged_2/road_damaged_2_nor_gl_1k.png",
        .roughnessPath =
            "textures/pbr/road_damaged_2/road_damaged_2_rough_1k.png",
        .normalStrength = 1.3f
    },
    {
        .name = "Rubber Tiles",
        .albedoPath =
            "textures/pbr/rubber_tiles/rubber_tiles_diff_1k.jpg",
        .normalPath =
            "textures/pbr/rubber_tiles/rubber_tiles_nor_gl_1k.png",
        .roughnessPath =
            "textures/pbr/rubber_tiles/rubber_tiles_rough_1k.png",
        .normalStrength = 1.0f
    }
}};
}

void SandboxScene::build(Scene &scene, ResourceManager &resources) {
    Entity &cameraEntity = scene.createEntity("Showcase Camera");
    Transform &cameraTransform =
            cameraEntity.getComponent<TransformComponent>().local;
    cameraTransform.position = {0.0f, 4.1f, 16.5f};
    cameraTransform.lookAt({0.0f, 4.0f, 0.0f});
    CameraComponent &camera = cameraEntity.addComponent<CameraComponent>();
    if (auto *perspective =
            std::get_if<PerspectiveProjection>(&camera.projection)) {
        perspective->fov = 43.0f;
        perspective->near = 0.1f;
        perspective->far = 80.0f;
    }
    scene.setActiveCamera(cameraEntity.id);

    const Mesh &planeMesh = resources.getPlaneMesh();
    const Mesh &cubeMesh = resources.getCubeMesh();
    const Mesh &sphereMesh = resources.getSphereMesh();
    const Texture2D &whiteTexture = resources.getWhiteTexture();
    const Shader &pbrShader = resources.getPBRShader();

    PBRMaterial &floorMaterial = resources.loadPBRMaterial(
        "showcase/studio-floor",
        pbrShader,
        whiteTexture
    );
    floorMaterial.baseColor = {0.12f, 0.13f, 0.15f, 1.0f};
    floorMaterial.metallic = 0.0f;
    floorMaterial.roughness = 0.72f;

    PBRMaterial &wallMaterial = resources.loadPBRMaterial(
        "showcase/studio-wall",
        pbrShader,
        whiteTexture
    );
    wallMaterial.baseColor = {0.055f, 0.062f, 0.075f, 1.0f};
    wallMaterial.metallic = 0.0f;
    wallMaterial.roughness = 0.92f;

    PBRMaterial &pedestalMaterial = resources.loadPBRMaterial(
        "showcase/pedestal",
        pbrShader,
        whiteTexture
    );
    pedestalMaterial.baseColor = {0.18f, 0.20f, 0.24f, 1.0f};
    pedestalMaterial.metallic = 0.15f;
    pedestalMaterial.roughness = 0.32f;

    const auto createMesh = [&](
        const std::string &name,
        const Mesh &mesh,
        const Material &material,
        const Transform &transform
    ) -> Entity & {
        Entity &entity = scene.createEntity(name);
        entity.getComponent<TransformComponent>().local = transform;
        entity.addComponent<MeshRendererComponent>(&mesh, &material);
        return entity;
    };

    (void)createMesh(
        "Studio Floor",
        planeMesh,
        floorMaterial,
        {
            .position = {0.0f, 0.0f, 0.5f},
            .rotation = {-90.0f, 0.0f, 0.0f},
            .scale = {20.0f, 16.0f, 1.0f}
        }
    );
    (void)createMesh(
        "Studio Backdrop",
        planeMesh,
        wallMaterial,
        {
            .position = {0.0f, 4.4f, -2.1f},
            .rotation = {0.0f, 0.0f, 0.0f},
            .scale = {20.0f, 10.5f, 1.0f}
        }
    );

    std::array<PBRMaterial *, ShowcaseMaterials.size()> materials{};
    for (std::size_t index = 0; index < ShowcaseMaterials.size(); ++index) {
        const ShowcaseMaterial &source = ShowcaseMaterials[index];
        const Texture2D &albedo = resources.loadTexture(source.albedoPath);
        const Texture2D &normal = resources.loadTexture(
            source.normalPath,
            TextureColorSpace::Linear
        );
        const Texture2D &roughness = resources.loadTexture(
            source.roughnessPath,
            TextureColorSpace::Linear
        );

        PBRMaterial &material = resources.loadPBRMaterial(
            "showcase/" + std::string(source.name),
            pbrShader,
            albedo
        );
        material.normalTexture = &normal;
        material.roughnessTexture = &roughness;
        material.metallicTexture = nullptr;
        material.aoTexture = source.aoPath
                                 ? &resources.loadTexture(
                                     source.aoPath,
                                     TextureColorSpace::Linear
                                   )
                                 : nullptr;
        material.baseColor = glm::vec4(1.0f);
        material.metallic = source.metallic ? 1.0f : 0.0f;
        material.roughness = 1.0f;
        material.ao = 1.0f;
        material.useNormalMapping = true;
        material.flipNormalY = true;
        material.normalStrength = source.normalStrength;
        materials[index] = &material;
    }

    constexpr std::array<float, 3> columnPositions{-4.5f, 0.0f, 4.5f};
    constexpr std::array<float, 3> rowPositions{6.75f, 4.05f, 1.35f};
    for (std::size_t index = 0; index < materials.size(); ++index) {
        const std::size_t row = index / columnPositions.size();
        const std::size_t column = index % columnPositions.size();
        const float x = columnPositions[column];
        const float y = rowPositions[row];

        (void)createMesh(
            std::string(ShowcaseMaterials[index].name) + " Pedestal",
            cubeMesh,
            pedestalMaterial,
            {
                .position = {x, y - 1.27f, 0.0f},
                .rotation = {0.0f, 0.0f, 0.0f},
                .scale = {3.05f, 0.42f, 2.1f}
            }
        );

        Entity &sphere = createMesh(
            std::string("Material - ") + ShowcaseMaterials[index].name,
            sphereMesh,
            *materials[index],
            {
                .position = {x, y, 0.35f},
                .rotation = {0.0f, static_cast<float>(index) * 19.0f, 0.0f},
                .scale = {2.2f, 2.2f, 2.2f}
            }
        );
        SpinComponent &spin = sphere.addComponent<SpinComponent>();
        spin.degreesPerSecond = {
            0.0f,
            4.0f + static_cast<float>(index % 3) * 1.5f,
            0.0f
        };
    }

    Entity &ambientEntity = scene.createEntity("Ambient Studio Fill");
    AmbientLightComponent &ambient =
            ambientEntity.addComponent<AmbientLightComponent>();
    ambient.color = {0.24f, 0.28f, 0.36f};
    ambient.intensity = 0.16f;

    Entity &sunEntity = scene.createEntity("Soft Directional Light");
    sunEntity.getComponent<TransformComponent>().local.rotation =
            {-48.0f, -32.0f, 0.0f};
    DirectionalLightComponent &sun =
            sunEntity.addComponent<DirectionalLightComponent>();
    sun.color = {0.82f, 0.88f, 1.0f};
    sun.intensity = 0.34f;
    sun.castShadows = true;

    const auto createPointLight = [&](
        const char *name,
        const glm::vec3 &position,
        const glm::vec3 &color,
        const float intensity,
        const float range,
        const bool castShadows
    ) {
        Entity &entity = scene.createEntity(name);
        entity.getComponent<TransformComponent>().local.position = position;
        PointLightComponent &light =
                entity.addComponent<PointLightComponent>();
        light.color = color;
        light.intensity = intensity;
        light.range = range;
        light.castShadows = castShadows;
    };

    createPointLight(
        "Warm Key Light",
        {-5.5f, 8.5f, 10.0f},
        {1.0f, 0.72f, 0.48f},
        6.5f,
        26.0f,
        true
    );
    createPointLight(
        "Cool Fill Light",
        {6.5f, 5.0f, 9.0f},
        {0.46f, 0.68f, 1.0f},
        4.5f,
        24.0f,
        false
    );
    createPointLight(
        "Neutral Front Fill",
        {0.0f, 4.5f, 13.0f},
        {1.0f, 0.96f, 0.90f},
        3.5f,
        30.0f,
        false
    );
    createPointLight(
        "Blue Rim Light",
        {0.0f, 7.5f, -0.5f},
        {0.30f, 0.52f, 1.0f},
        6.0f,
        16.0f,
        false
    );
}
