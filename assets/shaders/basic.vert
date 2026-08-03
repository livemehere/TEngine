#version 410 core

layout (std140) uniform CameraData {
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in mat4 aInstanceMatrix;

uniform mat4 uModel;

out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vPos;

void main()
{
    mat4 modelMatrix = uModel * aInstanceMatrix;

    gl_Position = camera.projection * camera.view * modelMatrix * vec4(aPos, 1.0);
//    gl_PointSize = 10.0;
    vTexCoord = aTexCoord;

    mat3 normalMatrix = mat3(transpose(inverse(modelMatrix)));
    vNormal = normalMatrix * aNormal;
    vPos = vec3(modelMatrix * vec4(aPos, 1.0));
}
