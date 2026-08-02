find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

add_library(TEngine STATIC
    src/common.h

    src/core/Application.cpp
    src/core/Application.h
    src/core/GameModule.h
    src/core/Input.cpp
    src/core/Input.h
    src/core/Window.cpp
    src/core/Window.h
    src/core/utils.h

    src/camera/CameraComponent.cpp
    src/camera/CameraComponent.h
    src/camera/FreeLookCameraController.cpp
    src/camera/FreeLookCameraController.h

    src/editor/ComponentDrawerRegistry.cpp
    src/editor/ComponentDrawerRegistry.h
    src/editor/ComponentDrawers.cpp
    src/editor/Editor.cpp
    src/editor/Editor.h
    src/editor/EditorTheme.cpp
    src/editor/EditorTheme.h

    src/graphics/Cubemap.cpp
    src/graphics/CubeMap.h
    src/graphics/FrameBuffer.cpp
    src/graphics/FrameBuffer.h
    src/graphics/Shader.cpp
    src/graphics/Shader.h
    src/graphics/ShaderStage.cpp
    src/graphics/ShaderStage.h
    src/graphics/Texture2D.cpp
    src/graphics/Texture2D.h

    src/rendering/Lights.h
    src/rendering/PostProcessor.cpp
    src/rendering/PostProcessor.h
    src/rendering/RenderExtent.h
    src/rendering/RenderQueue.h
    src/rendering/Renderer.cpp
    src/rendering/Renderer.h
    src/rendering/Transform.h

    src/rendering/mesh/Mesh.cpp
    src/rendering/mesh/Mesh.h
    src/rendering/mesh/MeshData.h
    src/rendering/mesh/MeshRendererComponent.h
    src/rendering/mesh/Vertex.h
    src/rendering/mesh/materials/LitMaterial.cpp
    src/rendering/mesh/materials/LitMaterial.h
    src/rendering/mesh/materials/Material.h
    src/rendering/mesh/materials/UnlitMaterial.cpp
    src/rendering/mesh/materials/UnlitMaterial.h
    src/rendering/mesh/primitives/PrimitiveMeshes.cpp
    src/rendering/mesh/primitives/PrimitiveMeshes.h
    src/rendering/model/Model.h
    src/rendering/model/ModelImporter.cpp
    src/rendering/model/ModelImporter.h
    src/rendering/skybox/SkyboxComponent.h

    src/resources/ResourceManager.cpp
    src/resources/ResourceManager.h

    src/scene/Behaviour.h
    src/scene/Component.h
    src/scene/ComponentTypeRegistry.cpp
    src/scene/ComponentTypeRegistry.h
    src/scene/Entity.h
    src/scene/EntityId.h
    src/scene/Scene.cpp
    src/scene/Scene.h
    src/scene/SceneSerializer.cpp
    src/scene/SceneSerializer.h
    src/scene/TransformComponent.h

    src/thirdparty/stb_image.cpp
    src/thirdparty/stb_image.h
)

target_include_directories(TEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(TEngine
    PUBLIC
        glfw
        glad::glad
        imgui::imgui
        glm::glm
        assimp::assimp
)

target_compile_definitions(TEngine
    PUBLIC
        GLM_ENABLE_EXPERIMENTAL
    PRIVATE
        ASSET_ROOT="${PROJECT_SOURCE_DIR}/assets"
)
