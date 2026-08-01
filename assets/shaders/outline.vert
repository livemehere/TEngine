#version 410 core

layout (std140) uniform CameraData {
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform float uOutlineWidth;

void main()
{
    vec3 worldPos = vec3(uModel * vec4(aPos, 1.0));

    mat3 normalMatrix = mat3(transpose(inverse(uModel)));

    vec3 worldNormal = normalize(normalMatrix * aNormal);

    worldPos += worldNormal * uOutlineWidth;

    gl_Position = camera.projection * camera.view * vec4(worldPos, 1.0);
}
