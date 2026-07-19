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

out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uColor;

vec3 calculateAmbient(vec3 albedo)
{
    vec3 color = lights.ambientLightColorIntensity.rgb;
    float intensity = lights.ambientLightColorIntensity.w;
    return albedo * color * intensity;
}

vec3 calculatePointLight(PointLight light, vec3 normal)
{
    /** diffuse */
    vec3 lightPos = light.positionRange.rgb;
    vec3 lightColor = light.colorIntensity.rgb;
    float lightIntensity = light.colorIntensity.w;
    vec3 lightDir = normalize(lightPos - vPos);
    float diff = max(dot(normal, lightDir),0);
    vec3 diffuse = lightColor * lightIntensity * diff;

    /** specular */
    // TODO:

    return diffuse;
}

void main()
{
    vec4 textureColor = texture(uTexture, vTexCoord);
    vec3 albedo = textureColor.rgb * uColor.rgb;

    /** ambient light */
    vec3 result = calculateAmbient(albedo);

    /** point lights */
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(camera.position.xyz - vPos);
    int pointLightCount = min(lights.lightCounts.y,MAX_POINT_LIGHTS);
    for(int i=0; i<pointLightCount; i++){
       result += calculatePointLight(lights.pointLights[i], normal);
    }


    // Phong Light
//    vec3 reflectDir = reflect(-lightDir, normal);
//
//
//    float spec = pow(max(dot(viewDir, reflectDir),0.0),32);
//    vec4 specular = vec4(specularIntensity * spec * lights.ambientLightColorIntensity.rgb, 1.0);

    float alpha = textureColor.a * uColor.a;
    FragColor = vec4(result, alpha);
    /** for normal direction debug */
//     FragColor = vec4(vNormal * 0.5 + 0.5, 0.8);
}