#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 3) in mat4 aInstanceMatrix;

uniform mat4 uModel;
uniform mat4 uShadowMatrix;

out vec3 vWorldPosition;

void main()
{
    vec4 worldPosition = uModel * aInstanceMatrix * vec4(aPos, 1.0);
    vWorldPosition = worldPosition.xyz;
    gl_Position = uShadowMatrix * worldPosition;
}
