#version 410 core

#define MAX_DIRECTIONAL_LIGHTS 4
#define MAX_POINT_LIGHTS 16
#define MAX_SPOT_LIGHTS 8

layout (std140) uniform CameraData {
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

struct DirectionalLight {
    // xyz, w(x)
    vec4 direction;
    // rgb, w
    vec4 colorIntensity;
};

struct PointLight {
    // xyz, w
    vec4 positionRange;
    // rgb, w
    vec4 colorIntensity;
};

struct SpotLight {
    vec4 direction;
    // xyz, w
    vec4 positionRange;
    // rgb, w
    vec4 colorIntensity;

    // x:inner, y:outer
    vec4 coneAngles;
};

layout (std140) uniform LightsData {
    // rgb : color
    // w : intensity
    vec4 ambientLightColorIntensity;

    // x: directionalLight / y : pointLight
    ivec4 lightCounts;
    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
    PointLight pointLights[MAX_POINT_LIGHTS];
    SpotLight spotLights[MAX_SPOT_LIGHTS];
} lights;

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vPos;

struct Material {
    sampler2D albedo;
    sampler2D specular;
    int hasSpecularMap;
    vec4 baseColor;
    float shininess;
    float specularStrength;
};

uniform Material material;

out vec4 FragColor;

vec3 calculateAmbient(vec3 albedo)
{
    vec3 color = lights.ambientLightColorIntensity.rgb;
    float intensity = lights.ambientLightColorIntensity.w;
    return albedo * color * intensity;
}

vec3 calculateDirectionalLight(DirectionalLight light, vec3 albedo, vec3 normal, vec3 viewDir, float specularMask)
{
    /** diffuse */
    vec3 lightDir = normalize(-light.direction.xyz);
    vec3 color = light.colorIntensity.rgb;
    float intensity = light.colorIntensity.w;
    float diffuseFactor = max(dot(normal, lightDir),0.0);
    vec3 diffuse = albedo * color * intensity * diffuseFactor;

    vec3 specular = vec3(0.0);
    if(diffuseFactor > 0.0){
        vec3 reflectDir = reflect(-lightDir, normal);
        float specularAngle = max(dot(reflectDir, viewDir), 0.0);
        float specularFactor = pow(specularAngle, material.shininess);
        specular = color * intensity * specularFactor * material.specularStrength * specularMask;
    }

    return diffuse + specular;
}

vec3 calculatePointLight(PointLight light, vec3 albedo, vec3 normal, vec3 viewDir, float specularMask)
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
    float diffuseFactor = max(dot(normal, lightDir), 0.0);
    float distanceNormal = distance / range;
    float attenuation = 1.0 - smoothstep(0.0, 1.0, distanceNormal);
    vec3 diffuse = albedo * color * intensity * diffuseFactor * attenuation;

    vec3 specular = vec3(0.0);
    if(diffuseFactor > 0.0){
        vec3 reflectDir = reflect(-lightDir, normal);
        float specularAngle = max(dot(viewDir, reflectDir),0.0);
        float specularFactor = pow(specularAngle, material.shininess);
        specular = color * intensity * attenuation * specularFactor * material.specularStrength * specularMask;
    }

    return diffuse + specular;
}

vec3 calculateSpotLight(SpotLight light, vec3 albedo, vec3 normal, vec3 viewDir, float specularMask)
{
    vec3 position = light.positionRange.xyz;
    float range = light.positionRange.w;

    vec3 toLight = position - vPos;
    float distance = length(toLight);
    if(distance > range){
        return vec3(0.0);
    }

    vec3 lightDir = toLight / max(distance, 0.0001);
    vec3 spotDirection = normalize(light.direction.xyz);

    float theta = dot(-lightDir, spotDirection);

    float innerCos = light.coneAngles.x;
    float outerCos = light.coneAngles.y;

    float coneFactor = smoothstep(outerCos, innerCos, theta);

    if(coneFactor <= 0.0){
        return vec3(0.0);
    }

    float distanceNormal = distance / range;
    float attenuation = 1.0 - smoothstep(0.0, 1.0, distanceNormal);

    /** diffuse */
    vec3 color = light.colorIntensity.rgb;
    float intensity = light.colorIntensity.w;
    float diffuseFactor = max(dot(normal, lightDir),0.0);

    vec3 diffuse = albedo * color * intensity * diffuseFactor * attenuation * coneFactor;

    vec3 specular = vec3(0.0);
    if(diffuseFactor > 0.0){
        vec3 reflectDir = reflect(-lightDir, normal);
        float specularAngle = max(dot(reflectDir, viewDir), 0.0);
        float specularFactor = pow(specularAngle, material.shininess);

        specular = color * intensity * coneFactor * specularFactor * material.specularStrength * specularMask;
    }

    return diffuse + specular;
}

void main()
{
    vec4 textureColor = texture(material.albedo, vTexCoord);
    vec3 albedo = textureColor.rgb * material.baseColor.rgb;

    /** ambient light */
    vec3 result = calculateAmbient(albedo);

    /** point lights */
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(camera.position.xyz - vPos);
    int pointLightCount = min(lights.lightCounts.y,MAX_POINT_LIGHTS);
    int directionalLightCount = min(lights.lightCounts.x,MAX_DIRECTIONAL_LIGHTS);
    int spotLightCount = min(lights.lightCounts.z,MAX_SPOT_LIGHTS);

    float specularMask = 0.0;
    if(material.hasSpecularMap != 0){
        specularMask = texture(material.specular,vTexCoord).r;
    }

    for(int i=0; i<directionalLightCount; i++){
        result += calculateDirectionalLight(lights.directionalLights[i], albedo, normal, viewDir, specularMask);
    }

    for(int i=0; i<pointLightCount; i++){
       result += calculatePointLight(lights.pointLights[i], albedo, normal, viewDir, specularMask);
    }

    for(int i=0; i< spotLightCount; i++){
        result += calculateSpotLight(lights.spotLights[i], albedo, normal, viewDir, specularMask);
    }

    float alpha = textureColor.a * material.baseColor.a;
    FragColor = vec4(result, alpha);
    /** for normal direction debug */
//     FragColor = vec4(vNormal * 0.5 + 0.5, 0.8);
}