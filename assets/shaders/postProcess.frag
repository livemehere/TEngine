#version 410 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uSceneTexture;

void main()
{
    FragColor = texture(
        uSceneTexture,
        vTexCoord
    );
}