#pragma once

#include "../core/Entity.h"


class Scene;

class Editor {
    std::optional<EntityId> selectedEntityId;
    std::optional<EntityMoveRequest> pendingMoveReq;

    void drawHierarchy(Scene& scene);
    void drawInspector(Scene& scene);
    void drawEntityNode(Scene& scene, const Entity& entity);
    void drawSiblingList(Scene& scene, std::optional<EntityId> id);
    void drawInsertionSlot(std::optional<EntityId> id, size_t insertIndex);
public:
    void draw(Scene& scene);
};
