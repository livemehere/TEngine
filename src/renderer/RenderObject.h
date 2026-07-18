#pragma once

#include "Mesh.h"
#include "../renderer/Transform.h"
#include "../graphics/Shader.h"
#include "../graphics/Texture2D.h"

struct RenderObject {
    Transform transform;
    Mesh* mesh;
    Shader* shader;
    Texture2D* texture;
    glm::vec4 color;
};
