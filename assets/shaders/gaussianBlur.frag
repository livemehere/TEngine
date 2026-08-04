#version 410 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uImage;
uniform int uHorizontal;

const float weights[5] = float[](
    0.227027,
    0.1945946,
    0.1216216,
    0.054054,
    0.016216
);

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(uImage, 0));
    vec3 result = texture(uImage, vTexCoord).rgb * weights[0];

    for (int index = 1; index < 5; ++index) {
        vec2 offset = uHorizontal != 0
            ? vec2(texelSize.x * index, 0.0)
            : vec2(0.0, texelSize.y * index);

        result += texture(uImage, vTexCoord + offset).rgb * weights[index];
        result += texture(uImage, vTexCoord - offset).rgb * weights[index];
    }

    FragColor = vec4(result, 1.0);
}
