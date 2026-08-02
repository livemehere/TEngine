add_library(Game STATIC
    src/Game.cpp
    src/Game.h
    src/components/TestComponent.cpp
    src/components/TestComponent.h
    src/scenes/SandboxScene.cpp
    src/scenes/SandboxScene.h
)

target_include_directories(Game
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Game
    PUBLIC
        TEngine
)
