#pragma once

#include <glad/glad.h>

#include "ComponentDrawerRegistry.h"
#include "../rendering/RenderExtent.h"
#include "../rendering/RenderSettings.h"
#include "../scene/Entity.h"

class Scene;
class ComponentTypeRegistry;
struct MouseState;
struct WindowSize;

enum class PlayModeRequest {
    Play,
    Stop
};


class Editor {
    struct EntityCreationRequest {
        std::optional<EntityId> parentId;
    };

    const ComponentTypeRegistry &componentTypes;
    ComponentDrawerRegistry componentDrawers;
    std::optional<EntityId> selectedEntityId;
    std::optional<EntityMoveRequest> pendingMoveReq;
    std::optional<EntityCreationRequest> pendingEntityCreation;
    std::optional<PlayModeRequest> pendingPlayModeRequest;

    void drawHierarchy(Scene& scene);
    void drawInspector(Scene& scene);
    void drawInspectorContent(Scene& scene);
    void drawDebug(
        Scene& scene,
        const WindowSize& windowSize,
        const MouseState& mouseState,
        RenderSettings& renderSettings
    );
    void drawEntityNode(Scene& scene, const Entity& entity);
    void drawSiblingList(Scene& scene, std::optional<EntityId> id);
    void drawInsertionSlot(std::optional<EntityId> id, size_t insertIndex);
public:
    explicit Editor(const ComponentTypeRegistry &componentTypes);

    std::optional<EntityId> getSelectedEntityId() const { return selectedEntityId; }
    std::optional<PlayModeRequest> consumePlayModeRequest();
    void beginFrame();
    void draw(
        Scene& scene,
        const WindowSize& windowSize,
        const MouseState& mouseState,
        RenderSettings& renderSettings
    );
    RenderExtent drawSceneView(GLuint textureId, bool isPlaying);
};
