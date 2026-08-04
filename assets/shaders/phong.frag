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

layout (std140) uniform DebugData {
    int viewMode;
    float depthNear;
    float depthFar;
    int padding;
} debugData;

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
    int useBlinnPhong;
    int environmentMappingMode;
    float environmentStrength;
    float refractiveIndex;
};

uniform Material material;
uniform samplerCube uEnvironmentMap;
uniform int uHasEnvironmentMap;
uniform sampler2D uShadowMap;
uniform mat4 uLightSpaceMatrix;
uniform int uShadowsEnabled;
uniform int uShadowLightIndex;
uniform float uShadowBiasMin;
uniform float uShadowBiasSlope;
uniform int uShadowPcfRadius;
uniform samplerCube uPointShadowMap;
uniform int uPointShadowsEnabled;
uniform int uPointShadowLightIndex;
uniform vec3 uPointShadowLightPosition;
uniform float uPointShadowFarPlane;
uniform float uPointShadowBias;
uniform float uPointShadowSoftness;
uniform int uPointShadowSampleCount;

const vec3 pointShadowSampleDirections[20] = vec3[](
    vec3( 1.0,  1.0,  1.0), vec3( 1.0, -1.0,  1.0),
    vec3(-1.0, -1.0,  1.0), vec3(-1.0,  1.0,  1.0),
    vec3( 1.0,  1.0, -1.0), vec3( 1.0, -1.0, -1.0),
    vec3(-1.0, -1.0, -1.0), vec3(-1.0,  1.0, -1.0),
    vec3( 1.0,  1.0,  0.0), vec3( 1.0, -1.0,  0.0),
    vec3(-1.0, -1.0,  0.0), vec3(-1.0,  1.0,  0.0),
    vec3( 1.0,  0.0,  1.0), vec3(-1.0,  0.0,  1.0),
    vec3( 1.0,  0.0, -1.0), vec3(-1.0,  0.0, -1.0),
    vec3( 0.0,  1.0,  1.0), vec3( 0.0, -1.0,  1.0),
    vec3( 0.0, -1.0, -1.0), vec3( 0.0,  1.0, -1.0)
);

out vec4 FragColor;

float calculateSpecularFactor(vec3 normal, vec3 lightDir, vec3 viewDir)
{
    float specularAngle;
    if(material.useBlinnPhong != 0){
        vec3 halfway = lightDir + viewDir;
        float halfwayLengthSquared = dot(halfway, halfway);
        if(halfwayLengthSquared <= 0.000001){
            return 0.0;
        }
        halfway *= inversesqrt(halfwayLengthSquared);
        specularAngle = max(dot(normal, halfway), 0.0);
    }else{
        vec3 reflectDir = reflect(-lightDir, normal);
        specularAngle = max(dot(reflectDir, viewDir), 0.0);
    }

    return pow(specularAngle, material.shininess);
}

float calculateDirectionalShadow(
    int lightIndex,
    vec3 normal,
    vec3 lightDir
)
{
    if(uShadowsEnabled == 0 || lightIndex != uShadowLightIndex){
        return 0.0;
    }

    vec4 lightSpacePosition = uLightSpaceMatrix * vec4(vPos, 1.0);
    vec3 projected = lightSpacePosition.xyz / lightSpacePosition.w;
    projected = projected * 0.5 + 0.5;

    if(projected.z <= 0.0 || projected.z >= 1.0){
        return 0.0;
    }

    float bias = max(
        uShadowBiasSlope * (1.0 - max(dot(normal, lightDir), 0.0)),
        uShadowBiasMin
    );
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
    int radius = clamp(uShadowPcfRadius, 0, 3);
    float shadow = 0.0;
    float sampleCount = 0.0;

    for(int x = -radius; x <= radius; ++x){
        for(int y = -radius; y <= radius; ++y){
            float closestDepth = texture(
                uShadowMap,
                projected.xy + vec2(x, y) * texelSize
            ).r;
            shadow += projected.z - bias > closestDepth ? 1.0 : 0.0;
            sampleCount += 1.0;
        }
    }

    return shadow / max(sampleCount, 1.0);
}

float calculatePointShadow(int lightIndex)
{
    if(uPointShadowsEnabled == 0 ||
       lightIndex != uPointShadowLightIndex){
        return 0.0;
    }

    vec3 fragmentToLight = vPos - uPointShadowLightPosition;
    float currentDepth = length(fragmentToLight);
    if(currentDepth <= 0.0 || currentDepth >= uPointShadowFarPlane){
        return 0.0;
    }

    int sampleCount = clamp(uPointShadowSampleCount, 1, 20);
    if(sampleCount == 1){
        float closestDepth = texture(
            uPointShadowMap,
            fragmentToLight
        ).r * uPointShadowFarPlane;
        return currentDepth - uPointShadowBias > closestDepth ? 1.0 : 0.0;
    }

    float viewDistance = length(camera.position.xyz - vPos);
    float diskRadius = uPointShadowSoftness *
                       (1.0 + viewDistance / uPointShadowFarPlane);
    float shadow = 0.0;
    for(int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex){
        float closestDepth = texture(
            uPointShadowMap,
            fragmentToLight +
                pointShadowSampleDirections[sampleIndex] * diskRadius
        ).r * uPointShadowFarPlane;
        shadow += currentDepth - uPointShadowBias > closestDepth
                    ? 1.0
                    : 0.0;
    }

    return shadow / float(sampleCount);
}

vec4 applyDebugView(vec4 shadedColor)
{
    if(debugData.viewMode == 1){
        float depth = -(camera.view * vec4(vPos, 1.0)).z;
        float depthRange = max(debugData.depthFar - debugData.depthNear, 0.0001);
        float gray = clamp((depth - debugData.depthNear) / depthRange, 0.0, 1.0);
        return vec4(vec3(gray), 1.0);
    }

    if(debugData.viewMode == 2){
        vec3 normal = normalize(vNormal);
        return vec4(normal * 0.5 + 0.5, 1.0);
    }

    return shadedColor;
}

vec3 calculateAmbient(vec3 albedo)
{
    vec3 color = lights.ambientLightColorIntensity.rgb;
    float intensity = lights.ambientLightColorIntensity.w;
    return albedo * color * intensity;
}

vec3 calculateDirectionalLight(
    int lightIndex,
    DirectionalLight light,
    vec3 albedo,
    vec3 normal,
    vec3 viewDir,
    float specularMask
)
{
    /** diffuse */
    vec3 lightDir = normalize(-light.direction.xyz);
    vec3 color = light.colorIntensity.rgb;
    float intensity = light.colorIntensity.w;
    float diffuseFactor = max(dot(normal, lightDir),0.0);
    vec3 diffuse = albedo * color * intensity * diffuseFactor;

    vec3 specular = vec3(0.0);
    if(diffuseFactor > 0.0){
        float specularFactor = calculateSpecularFactor(normal, lightDir, viewDir);
        specular = color * intensity * specularFactor * material.specularStrength * specularMask;
    }

    float shadow = calculateDirectionalShadow(
        lightIndex,
        normal,
        lightDir
    );
    return (diffuse + specular) * (1.0 - shadow);
}

vec3 calculatePointLight(
    int lightIndex,
    PointLight light,
    vec3 albedo,
    vec3 normal,
    vec3 viewDir,
    float specularMask
)
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
        float specularFactor = calculateSpecularFactor(normal, lightDir, viewDir);
        specular = color * intensity * attenuation * specularFactor * material.specularStrength * specularMask;
    }

    float shadow = calculatePointShadow(lightIndex);
    return (diffuse + specular) * (1.0 - shadow);
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
        float specularFactor = calculateSpecularFactor(normal, lightDir, viewDir);

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
        result += calculateDirectionalLight(
            i,
            lights.directionalLights[i],
            albedo,
            normal,
            viewDir,
            specularMask
        );
    }

    for(int i=0; i<pointLightCount; i++){
       result += calculatePointLight(
           i,
           lights.pointLights[i],
           albedo,
           normal,
           viewDir,
           specularMask
       );
    }

    for(int i=0; i< spotLightCount; i++){
        result += calculateSpotLight(lights.spotLights[i], albedo, normal, viewDir, specularMask);
    }

    if(uHasEnvironmentMap != 0 && material.environmentStrength > 0.0){
        vec3 incident = normalize(vPos - camera.position.xyz);
        vec3 environmentDirection;

        if(material.environmentMappingMode == 1){
            float eta = 1.0 / max(material.refractiveIndex, 1.0);
            environmentDirection = refract(incident, normal, eta);
        }else{
            environmentDirection = reflect(incident, normal);
        }

        vec3 environmentColor = texture(uEnvironmentMap, environmentDirection).rgb;
        float environmentStrength = clamp(material.environmentStrength, 0.0, 1.0);
        result = mix(result, environmentColor, environmentStrength);
    }

    float alpha = textureColor.a * material.baseColor.a;
    float distanceToCamera = length(camera.position.xyz - vPos);
    float fogStart = 40.0;
    float fogEnd = 100.0;
    float fogAmount = smoothstep(fogStart, fogEnd, distanceToCamera);

    vec3 fogColor = vec3(0.5, 0.6,0.7);
    vec3 finalColor = mix(result, fogColor, fogAmount);
    FragColor = applyDebugView(vec4(finalColor, alpha));
}
