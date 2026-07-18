#version 410 core

in vec2 vTexCoord;
in vec3 vNormal;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uColor;

void main()
{
//    FragColor = texture(uTexture, vTexCoord) * uColor;
    /** for normal direction debug */
     FragColor = vec4(vNormal * 0.5 + 0.5, 0.8);
}