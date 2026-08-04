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
layout (location = 7) in vec4 aTangent;

uniform mat4 uModel;

out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vPos;
out mat3 vTBN;

void main()
{
    mat4 modelMatrix = uModel * aInstanceMatrix;

    gl_Position = camera.projection * camera.view * modelMatrix * vec4(aPos, 1.0);
//    gl_PointSize = 10.0;
    vTexCoord = aTexCoord;

    mat3 normalMatrix = mat3(transpose(inverse(modelMatrix)));
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(mat3(modelMatrix) * aTangent.xyz);
    T = normalize(T - N * dot(N, T));
    vec3 B = normalize(cross(N, T)) * aTangent.w;

    vNormal = N;
    vTBN = mat3(T, B, N);
    vPos = vec3(modelMatrix * vec4(aPos, 1.0));
}
