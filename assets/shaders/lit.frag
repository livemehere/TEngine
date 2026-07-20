#version 410 core

#define MAX_POINT_LIGHTS 16

layout (std140) uniform CameraData {
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

struct PointLight {
    // xyz, w
    vec4 positionRange;
    // rgb, w
    vec4 colorIntensity;
};

layout (std140) uniform LightsData {
    // rgb : color
    // w : intensity
    vec4 ambientLightColorIntensity;

    // x: directionalLight / y : pointLight
    ivec4 lightCounts;
    PointLight pointLights[MAX_POINT_LIGHTS];
} lights;

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vPos;

uniform sampler2D uAlbedoTexture;
uniform vec4 uBaseColor;
uniform float uShiniess = 32.0;
uniform float uSpecularStrength = 1.0;

out vec4 FragColor;

vec3 calculateAmbient(vec3 albedo)
{
    vec3 color = lights.ambientLightColorIntensity.rgb;
    float intensity = lights.ambientLightColorIntensity.w;
    return albedo * color * intensity;
}

vec3 calculatePointLight(PointLight light, vec3 albedo, vec3 normal, vec3 viewDir)
{
    /** diffuse */
    vec3 color = light.colorIntensity.rgb;
    float intensity = light.colorIntensity.w;

    vec3 position = light.positionRange.rgb;
    float range = light.positionRange.w;
    vec3 toLight = position - vPos;
    float distance = length(toLight);

    if(distance > range){
        return vec3(0.0);
    }

    vec3 lightDir = toLight / max(distance, 0.0001);
    float diffuseFactor = max(dot(normal, lightDir), 0);
    float distanceNormal = distance / range;
    float attenuation = 1.0 - smoothstep(0.0, 1.0, distanceNormal);

    vec3 diffuse = albedo * color * intensity * diffuseFactor * attenuation;

    /** specular */
    vec3 reflectDir = reflect(-lightDir, normal);
    float specularAngle = max(dot(viewDir, reflectDir),0.0);
    float specularFactor = pow(specularAngle, uShiniess);

    vec3 specular = color * intensity * attenuation * specularFactor * uSpecularStrength;

    return diffuse + specular;
}

void main()
{
    vec4 textureColor = texture(uAlbedoTexture, vTexCoord);
    vec3 albedo = textureColor.rgb * uBaseColor.rgb;

    /** ambient light */
    vec3 result = calculateAmbient(albedo);

    /** point lights */
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(camera.position.xyz - vPos);
    int pointLightCount = min(lights.lightCounts.y,MAX_POINT_LIGHTS);
    for(int i=0; i<pointLightCount; i++){
       result += calculatePointLight(lights.pointLights[i], albedo, normal, viewDir);
    }


    // Phong Light
//    vec3 reflectDir = reflect(-lightDir, normal);
//
//
//    float spec = pow(max(dot(viewDir, reflectDir),0.0),32);
//    vec4 specular = vec4(specularIntensity * spec * lights.ambientLightColorIntensity.rgb, 1.0);

    float alpha = textureColor.a * uBaseColor.a;
    FragColor = vec4(result, alpha);
    /** for normal direction debug */
//     FragColor = vec4(vNormal * 0.5 + 0.5, 0.8);
}