#version 410 core

layout (std140) uniform CameraData {
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vPos;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uColor;

void main()
{
    FragColor = texture(uTexture, vTexCoord) * uColor;
    /** for normal direction debug */
//     FragColor = vec4(vNormal * 0.5 + 0.5, 0.8);
}