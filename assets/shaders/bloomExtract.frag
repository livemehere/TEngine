#version 410 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uSceneTexture;
uniform float uThreshold;

void main()
{
    vec3 sceneColor = max(
        texture(uSceneTexture, vTexCoord).rgb,
        vec3(0.0)
    );
    float brightness = dot(
        sceneColor,
        vec3(0.2126, 0.7152, 0.0722)
    );

    FragColor = vec4(
        brightness > max(uThreshold, 0.0)
            ? sceneColor
            : vec3(0.0),
        1.0
    );
}
