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
    vec4 direction;
    vec4 colorIntensity;
};

struct PointLight {
    vec4 positionRange;
    vec4 colorIntensity;
};

struct SpotLight {
    vec4 direction;
    vec4 positionRange;
    vec4 colorIntensity;
    vec4 coneAngles;
};

layout (std140) uniform LightsData {
    vec4 ambientLightColorIntensity;
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
    sampler2D normalMap;
    sampler2D metallicMap;
    sampler2D roughnessMap;
    sampler2D aoMap;
    vec4 baseColor;
    float metallic;
    float roughness;
    float ao;
    int hasNormalMap;
    int hasMetallicMap;
    int hasRoughnessMap;
    int hasAOMap;
    int metallicChannel;
    int roughnessChannel;
    int aoChannel;
    int flipNormalY;
    float normalStrength;
};

uniform Material material;
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

out vec4 FragColor;

const float PI = 3.14159265359;
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

float channelValue(vec4 value, int channel)
{
    if (channel == 1) return value.g;
    if (channel == 2) return value.b;
    if (channel == 3) return value.a;
    return value.r;
}

vec3 getSurfaceNormal()
{
    if (material.hasNormalMap == 0) {
        return normalize(vNormal);
    }
    vec3 tangentNormal = texture(material.normalMap, vTexCoord).rgb * 2.0 - 1.0;
    tangentNormal.xy *= max(material.normalStrength, 0.0);
    if (material.flipNormalY != 0) {
        tangentNormal.y = -tangentNormal.y;
    }
    return normalize(vTBN * normalize(tangentNormal));
}

float distributionGGX(vec3 normal, vec3 halfway, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSquared = alpha * alpha;
    float normalDotHalfway = max(dot(normal, halfway), 0.0);
    float denominator = normalDotHalfway * normalDotHalfway *
                        (alphaSquared - 1.0) + 1.0;
    return alphaSquared /
           max(PI * denominator * denominator, 0.000001);
}

float geometrySchlickGGX(float normalDotDirection, float roughness)
{
    float value = roughness + 1.0;
    float k = value * value / 8.0;
    return normalDotDirection /
           max(normalDotDirection * (1.0 - k) + k, 0.000001);
}

float geometrySmith(
    vec3 normal,
    vec3 viewDir,
    vec3 lightDir,
    float roughness
)
{
    return geometrySchlickGGX(max(dot(normal, viewDir), 0.0), roughness) *
           geometrySchlickGGX(max(dot(normal, lightDir), 0.0), roughness);
}

vec3 fresnelSchlick(float cosine, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosine, 0.0, 1.0), 5.0);
}

vec3 evaluatePBR(
    vec3 albedo,
    float metallic,
    float roughness,
    vec3 normal,
    vec3 viewDir,
    vec3 lightDir,
    vec3 radiance
)
{
    vec3 halfway = normalize(viewDir + lightDir);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 fresnel = fresnelSchlick(max(dot(halfway, viewDir), 0.0), f0);
    float distribution = distributionGGX(normal, halfway, roughness);
    float geometry = geometrySmith(normal, viewDir, lightDir, roughness);
    float denominator = 4.0 *
                        max(dot(normal, viewDir), 0.0) *
                        max(dot(normal, lightDir), 0.0);
    vec3 specular = distribution * geometry * fresnel /
                    max(denominator, 0.0001);
    vec3 diffuseWeight = (vec3(1.0) - fresnel) * (1.0 - metallic);
    return (diffuseWeight * albedo / PI + specular) *
           radiance * max(dot(normal, lightDir), 0.0);
}

float calculateDirectionalShadow(
    int lightIndex,
    vec3 normal,
    vec3 lightDir
)
{
    if (uShadowsEnabled == 0 || lightIndex != uShadowLightIndex) {
        return 0.0;
    }
    vec4 lightSpacePosition = uLightSpaceMatrix * vec4(vPos, 1.0);
    vec3 projected = lightSpacePosition.xyz / lightSpacePosition.w;
    projected = projected * 0.5 + 0.5;
    if (projected.z <= 0.0 || projected.z >= 1.0) {
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
    for (int x = -radius; x <= radius; ++x) {
        for (int y = -radius; y <= radius; ++y) {
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
    if (uPointShadowsEnabled == 0 ||
        lightIndex != uPointShadowLightIndex) {
        return 0.0;
    }
    vec3 fragmentToLight = vPos - uPointShadowLightPosition;
    float currentDepth = length(fragmentToLight);
    if (currentDepth <= 0.0 || currentDepth >= uPointShadowFarPlane) {
        return 0.0;
    }
    int sampleCount = clamp(uPointShadowSampleCount, 1, 20);
    if (sampleCount == 1) {
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
    for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
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
    if (debugData.viewMode == 1) {
        float depth = -(camera.view * vec4(vPos, 1.0)).z;
        float range = max(debugData.depthFar - debugData.depthNear, 0.0001);
        float gray = clamp(
            (depth - debugData.depthNear) / range,
            0.0,
            1.0
        );
        return vec4(vec3(gray), 1.0);
    }
    if (debugData.viewMode == 2) {
        return vec4(normalize(surfaceNormal) * 0.5 + 0.5, 1.0);
    }
    return shadedColor;
}

void main()
{
    vec3 albedo = texture(material.albedo, vTexCoord).rgb *
                  material.baseColor.rgb;
    float metallic = material.hasMetallicMap != 0
        ? channelValue(
            texture(material.metallicMap, vTexCoord),
            material.metallicChannel
          ) * material.metallic
        : material.metallic;
    float roughness = material.hasRoughnessMap != 0
        ? channelValue(
            texture(material.roughnessMap, vTexCoord),
            material.roughnessChannel
          ) * material.roughness
        : material.roughness;
    float ao = material.hasAOMap != 0
        ? channelValue(
            texture(material.aoMap, vTexCoord),
            material.aoChannel
          ) * material.ao
        : material.ao;
    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.04, 1.0);
    ao = clamp(ao, 0.0, 1.0);

    vec3 normal = getSurfaceNormal();
    vec3 viewDir = normalize(camera.position.xyz - vPos);
    vec3 result = albedo *
                  lights.ambientLightColorIntensity.rgb *
                  lights.ambientLightColorIntensity.w * ao;

    int directionalCount = min(
        lights.lightCounts.x,
        MAX_DIRECTIONAL_LIGHTS
    );
    for (int index = 0; index < directionalCount; ++index) {
        DirectionalLight light = lights.directionalLights[index];
        vec3 lightDir = normalize(-light.direction.xyz);
        vec3 radiance = light.colorIntensity.rgb * light.colorIntensity.w;
        float shadow = calculateDirectionalShadow(index, normal, lightDir);
        result += evaluatePBR(
            albedo,
            metallic,
            roughness,
            normal,
            viewDir,
            lightDir,
            radiance
        ) * (1.0 - shadow);
    }

    int pointCount = min(lights.lightCounts.y, MAX_POINT_LIGHTS);
    for (int index = 0; index < pointCount; ++index) {
        PointLight light = lights.pointLights[index];
        vec3 toLight = light.positionRange.xyz - vPos;
        float distance = length(toLight);
        if (distance > light.positionRange.w) {
            continue;
        }
        vec3 lightDir = toLight / max(distance, 0.0001);
        float rangeFade = 1.0 - smoothstep(
            0.0,
            1.0,
            distance / light.positionRange.w
        );
        vec3 radiance = light.colorIntensity.rgb *
                        light.colorIntensity.w * rangeFade;
        float shadow = calculatePointShadow(index);
        result += evaluatePBR(
            albedo,
            metallic,
            roughness,
            normal,
            viewDir,
            lightDir,
            radiance
        ) * (1.0 - shadow);
    }

    int spotCount = min(lights.lightCounts.z, MAX_SPOT_LIGHTS);
    for (int index = 0; index < spotCount; ++index) {
        SpotLight light = lights.spotLights[index];
        vec3 toLight = light.positionRange.xyz - vPos;
        float distance = length(toLight);
        if (distance > light.positionRange.w) {
            continue;
        }
        vec3 lightDir = toLight / max(distance, 0.0001);
        float coneFactor = smoothstep(
            light.coneAngles.y,
            light.coneAngles.x,
            dot(-lightDir, normalize(light.direction.xyz))
        );
        float rangeFade = 1.0 - smoothstep(
            0.0,
            1.0,
            distance / light.positionRange.w
        );
        vec3 radiance = light.colorIntensity.rgb *
                        light.colorIntensity.w * coneFactor * rangeFade;
        result += evaluatePBR(
            albedo,
            metallic,
            roughness,
            normal,
            viewDir,
            lightDir,
            radiance
        );
    }

    float distanceToCamera = length(camera.position.xyz - vPos);
    float fogAmount = smoothstep(40.0, 100.0, distanceToCamera);
    vec3 finalColor = mix(result, vec3(0.5, 0.6, 0.7), fogAmount);
    FragColor = applyDebugView(vec4(finalColor, 1.0), normal);
}
