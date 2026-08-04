#pragma once

#include <glad/glad.h>
#include <imgui.h>

#include "ComponentDrawerRegistry.h"
#include "InstanceSelection.h"
#include "SceneViewport.h"
#include "../rendering/RenderExtent.h"
#include "../rendering/RenderSettings.h"
#include "../rendering/RenderStats.h"
#include "../scene/Entity.h"

class Scene;
class ComponentTypeRegistry;
class ResourceManager;
class Mesh;
struct Model;
struct MouseState;
struct WindowSize;

enum class PlayModeRequest {
    Play,
    Stop
};


class Editor {
    enum class GizmoOperation {
        Translate,
        Rotate,
        Scale
    };

    enum class EntityCreationType {
        Empty,
        InstancedMesh,
        InstancedModel
    };

    struct EntityCreationRequest {
        std::optional<EntityId> parentId;
        EntityCreationType type = EntityCreationType::Empty;
        const Mesh *mesh = nullptr;
        const Model *model = nullptr;
        std::string name = "Entity";
    };

    const ComponentTypeRegistry &componentTypes;
    ResourceManager &resources;
    ComponentDrawerRegistry componentDrawers;
    std::optional<EntityId> selectedEntityId;
    std::optional<InstanceSelection> selectedInstance;
    std::optional<EntityMoveRequest> pendingMoveReq;
    std::optional<EntityCreationRequest> pendingEntityCreation;
    std::optional<EntityId> pendingEntityDeletion;
    std::optional<PlayModeRequest> pendingPlayModeRequest;
    GizmoOperation gizmoOperation = GizmoOperation::Translate;
    SceneViewport sceneViewport;

    void drawHierarchy(Scene& scene);
    void drawInspector(Scene& scene);
    void drawInspectorContent(Scene& scene);
    void drawDebug(
        Scene& scene,
        const WindowSize& windowSize,
        const MouseState& mouseState,
        RenderSettings& renderSettings,
        const RenderStats& renderStats
    );
    void drawEntityNode(Scene& scene, const Entity& entity);
    void drawSiblingList(Scene& scene, std::optional<EntityId> id);
    void drawInsertionSlot(std::optional<EntityId> id, size_t insertIndex);
    void drawEntityCreationMenu(std::optional<EntityId> parentId);
    void drawSelectionGizmo(
        Scene &scene,
        ImVec2 imageMin,
        ImVec2 imageSize,
        const RenderExtent &extent
    );
    void pickScene(Scene &scene, const MouseState &mouseState);
public:
    Editor(
        const ComponentTypeRegistry &componentTypes,
        ResourceManager &resources
    );

    std::optional<EntityId> getSelectedEntityId() const { return selectedEntityId; }
    std::optional<PlayModeRequest> consumePlayModeRequest();
    void beginFrame();
    void draw(
        Scene& scene,
        const WindowSize& windowSize,
        const MouseState& mouseState,
        RenderSettings& renderSettings,
        const RenderStats& renderStats
    );
    RenderExtent drawSceneView(
        Scene &scene,
        GLuint textureId,
        bool isPlaying,
        const MouseState &mouseState
    );
};
