#pragma once

#include "../opengl/Mesh.h"
#include "../common.h"
#include "../opengl/Shader.h"
#include "../opengl/Texture2D.h"

struct RenderObject {
    Transform transform;
    Mesh* mesh;
    Shader* shader;
    Texture2D* texture;
};
