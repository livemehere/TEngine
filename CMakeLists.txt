cmake_minimum_required(VERSION 4.2)
project(ElementalsProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)



add_executable(Elementals src/main.cpp
        src/Window.cpp
        src/Window.h
        src/World.cpp
        src/World.h
        src/utils.h
        src/Input.cpp
        src/Input.h
        src/common.h
        src/camera/Camera.cpp
        src/camera/Camera.h
        src/camera/FreeLookCameraController.cpp
        src/camera/FreeLookCameraController.h
)

target_link_libraries(Elementals PRIVATE
        glfw
        glad::glad
        imgui::imgui
        glm::glm
)

# TODO: COPY asset file to packaging.
target_compile_definitions(Elementals PRIVATE
        ASSET_ROOT="${CMAKE_SOURCE_DIR}/assets"
)

