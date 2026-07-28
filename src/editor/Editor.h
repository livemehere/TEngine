#pragma once

#include "../core/Entity.h"


class Scene;

class Editor {
    std::optional<EntityId> selectedEntityId;

    void drawHierarchy(Scene& scene);
    void drawInspector(Scene& scene);
public:
    void draw(Scene& scene);
};
