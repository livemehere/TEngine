#version 410 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uSceneTexture;
uniform int uGammaCorrectionEnabled;
uniform float uGamma;

void main()
{
    vec4 sceneColor = texture(
        uSceneTexture,
        vTexCoord
    );

    if(uGammaCorrectionEnabled != 0){
        float safeGamma = max(uGamma, 0.0001);
        sceneColor.rgb = pow(
            max(sceneColor.rgb, vec3(0.0)),
            vec3(1.0 / safeGamma)
        );
    }

    FragColor = sceneColor;
}
