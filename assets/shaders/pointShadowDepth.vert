#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 3) in mat4 aInstanceMatrix;

uniform mat4 uModel;

void main()
{
    gl_Position = uModel * aInstanceMatrix * vec4(aPos, 1.0);
}
