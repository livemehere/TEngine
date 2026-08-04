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
in mat3 vTBN;

struct Material {
    sampler2D albedo;
    sampler2D specular;
    sampler2D normalMap;
    sampler2D depthMap;
    int hasSpecularMap;
    int hasNormalMap;
    int hasDepthMap;
    vec4 baseColor;
    float shininess;
    float specularStrength;
    int useBlinnPhong;
    int flipNormalY;
    float normalStrength;
    int parallaxMode;
    float parallaxScale;
    int parallaxMinLayers;
    int parallaxMaxLayers;
    int discardParallaxEdges;
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

vec4 applyDebugView(vec4 shadedColor, vec3 surfaceNormal)
{
    if(debugData.viewMode == 1){
        float depth = -(camera.view * vec4(vPos, 1.0)).z;
        float depthRange = max(debugData.depthFar - debugData.depthNear, 0.0001);
        float gray = clamp((depth - debugData.depthNear) / depthRange, 0.0, 1.0);
        return vec4(vec3(gray), 1.0);
    }

    if(debugData.viewMode == 2){
        return vec4(normalize(surfaceNormal) * 0.5 + 0.5, 1.0);
    }

    return shadedColor;
}

vec2 getParallaxTexCoords(vec2 texCoords, vec3 tangentViewDir)
{
    if(material.hasDepthMap == 0 || material.parallaxMode == 0){
        return texCoords;
    }

    float viewDepth = max(tangentViewDir.z, 0.05);
    if(material.parallaxMode == 1){
        float depth = texture(material.depthMap, texCoords).r;
        vec2 offset = tangentViewDir.xy / viewDepth;
        return texCoords - offset * (depth * material.parallaxScale);
    }

    float minLayers = float(clamp(material.parallaxMinLayers, 1, 64));
    float maxLayers = float(clamp(material.parallaxMaxLayers, 1, 64));
    maxLayers = max(maxLayers, minLayers);
    float layerCount = mix(
        maxLayers,
        minLayers,
        clamp(abs(tangentViewDir.z), 0.0, 1.0)
    );
    float layerDepth = 1.0 / layerCount;
    vec2 totalOffset = tangentViewDir.xy * material.parallaxScale;
    vec2 deltaTexCoords = totalOffset / layerCount;

    vec2 currentTexCoords = texCoords;
    float currentLayerDepth = 0.0;
    float currentMapDepth = texture(
        material.depthMap,
        currentTexCoords
    ).r;

    for(int layer = 0; layer < 64; ++layer){
        if(currentLayerDepth >= currentMapDepth ||
           float(layer) >= layerCount){
            break;
        }
        currentTexCoords -= deltaTexCoords;
        currentMapDepth = texture(
            material.depthMap,
            currentTexCoords
        ).r;
        currentLayerDepth += layerDepth;
    }

    if(material.parallaxMode == 2){
        return currentTexCoords;
    }

    vec2 previousTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentMapDepth - currentLayerDepth;
    float beforeDepth = texture(
        material.depthMap,
        previousTexCoords
    ).r - currentLayerDepth + layerDepth;
    float denominator = afterDepth - beforeDepth;
    float weight = abs(denominator) > 0.000001
        ? clamp(afterDepth / denominator, 0.0, 1.0)
        : 0.0;
    return previousTexCoords * weight +
           currentTexCoords * (1.0 - weight);
}

vec3 getSurfaceNormal(vec2 texCoords)
{
    if(material.hasNormalMap == 0){
        return normalize(vNormal);
    }

    vec3 tangentNormal = texture(material.normalMap, texCoords).rgb;
    tangentNormal = tangentNormal * 2.0 - 1.0;
    tangentNormal.xy *= max(material.normalStrength, 0.0);
    if(material.flipNormalY != 0){
        tangentNormal.y = -tangentNormal.y;
    }
    return normalize(vTBN * normalize(tangentNormal));
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
    vec3 viewDir = normalize(camera.position.xyz - vPos);
    vec3 tangentViewDir = normalize(transpose(vTBN) * viewDir);
    vec2 texCoords = getParallaxTexCoords(vTexCoord, tangentViewDir);
    if(material.hasDepthMap != 0 &&
       material.discardParallaxEdges != 0 &&
       (texCoords.x < 0.0 || texCoords.x > 1.0 ||
        texCoords.y < 0.0 || texCoords.y > 1.0)){
        discard;
    }

    vec4 textureColor = texture(material.albedo, texCoords);
    vec3 albedo = textureColor.rgb * material.baseColor.rgb;

    /** ambient light */
    vec3 result = calculateAmbient(albedo);

    /** point lights */
    vec3 normal = getSurfaceNormal(texCoords);
    int pointLightCount = min(lights.lightCounts.y,MAX_POINT_LIGHTS);
    int directionalLightCount = min(lights.lightCounts.x,MAX_DIRECTIONAL_LIGHTS);
    int spotLightCount = min(lights.lightCounts.z,MAX_SPOT_LIGHTS);

    float specularMask = 0.0;
    if(material.hasSpecularMap != 0){
        specularMask = texture(material.specular,texCoords).r;
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
    FragColor = applyDebugView(vec4(finalColor, alpha), normal);
}
