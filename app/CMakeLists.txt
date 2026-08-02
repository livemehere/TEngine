add_executable(TEngineApp
        main.cpp
)

target_link_libraries(TEngineApp
        PRIVATE
        Game
)

set_target_properties(TEngineApp PROPERTIES
        OUTPUT_NAME TEngine
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
