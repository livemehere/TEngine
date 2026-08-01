#version 410 core

in vec2 vTexCoord;

out vec4 FragColor;

uniform sampler2D uAlbedoTexture;
uniform vec4 uBaseColor;

void main()
{
    vec4 color = texture(uAlbedoTexture, vTexCoord) * uBaseColor;
    FragColor = color;
}