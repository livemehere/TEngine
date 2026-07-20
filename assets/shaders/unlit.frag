#version 410 core

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vPos;

out vec4 FragColor;

uniform sampler2D uAlbedoTexture;
uniform vec4 uBaseColor;

void main()
{
    FragColor = texture(uAlbedoTexture, vTexCoord) * uBaseColor;
    /** for normal direction debug */
//     FragColor = vec4(vNormal * 0.5 + 0.5, 0.8);
}