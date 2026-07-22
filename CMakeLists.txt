cmake_minimum_required(VERSION 4.2)
project(TEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

add_executable(TEngine src/main.cpp
        src/core/Window.cpp
        src/core/Window.h
        src/Scene.cpp
        src/Scene.h
        src/core/utils.h
        src/core/Input.cpp
        src/core/Input.h
        src/common.h
        src/camera/Camera.cpp
        src/camera/Camera.h
        src/camera/FreeLookCameraController.cpp
        src/camera/FreeLookCameraController.h
        src/graphics/ShaderStage.cpp
        src/graphics/ShaderStage.h
        src/graphics/Shader.cpp
        src/graphics/Shader.h
        src/graphics/Texture2D.cpp
        src/graphics/Texture2D.h
        src/rendering/mesh/Mesh.cpp
        src/rendering/mesh/Mesh.h
        src/rendering/mesh/MeshRenderObject.h
        src/rendering/Transform.h
        src/rendering/mesh/materials/Material.h
        src/rendering/mesh/materials/LitMaterial.cpp
        src/rendering/mesh/materials/LitMaterial.h
        src/rendering/mesh/MeshRenderer.cpp
        src/rendering/mesh/MeshRenderer.h
        src/resources/ResourceManager.cpp
        src/resources/ResourceManager.h
        src/rendering/mesh/primitives/PrimitiveMeshes.cpp
        src/rendering/mesh/primitives/PrimitiveMeshes.h
        src/rendering/mesh/MeshData.h
        src/rendering/mesh/Vertex.h
        src/rendering/Lights.h
        src/rendering/Renderer.cpp
        src/rendering/Renderer.h
        src/rendering/mesh/materials/UnlitMaterial.cpp
        src/rendering/mesh/materials/UnlitMaterial.h
        src/thirdparty/stb_image.cpp
)

target_link_libraries(TEngine PRIVATE
        glfw
        glad::glad
        imgui::imgui
        glm::glm
        assimp::assimp
)

# TODO: COPY asset file to packaging.
target_compile_definitions(TEngine PRIVATE
        ASSET_ROOT="${CMAKE_SOURCE_DIR}/assets"
)

