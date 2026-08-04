#version 410 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uSceneTexture;
uniform sampler2D uBloomTexture;
uniform int uHdrEnabled;
uniform int uToneMappingMode;
uniform float uExposure;
uniform int uBloomEnabled;
uniform float uBloomStrength;
uniform int uGammaCorrectionEnabled;
uniform float uGamma;

void main()
{
    vec4 sceneSample = texture(
        uSceneTexture,
        vTexCoord
    );
    vec3 sceneColor = max(sceneSample.rgb, vec3(0.0));

    if (uBloomEnabled != 0) {
        vec3 bloomColor = max(
            texture(uBloomTexture, vTexCoord).rgb,
            vec3(0.0)
        );
        sceneColor += bloomColor * max(uBloomStrength, 0.0);
    }

    if(uHdrEnabled != 0){
        if(uToneMappingMode == 0){
            sceneColor = sceneColor / (sceneColor + vec3(1.0));
        }else{
            float exposure = max(uExposure, 0.0);
            sceneColor = vec3(1.0) - exp(-sceneColor * exposure);
        }
    }

    if(uGammaCorrectionEnabled != 0){
        float safeGamma = max(uGamma, 0.0001);
        sceneColor = pow(
            sceneColor,
            vec3(1.0 / safeGamma)
        );
    }

    FragColor = vec4(sceneColor, sceneSample.a);
}
