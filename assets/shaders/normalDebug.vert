#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uModel;

out VS_OUT {
    vec3 worldNormal;
} vertexOut;

void main()
{
    gl_Position = uModel * vec4(aPos, 1.0);

    mat3 normalMatrix = mat3(transpose(inverse(uModel)));
    vertexOut.worldNormal = normalize(normalMatrix * aNormal);
}
