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
out vec4 FragColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gMaterial;
uniform sampler2D uSSAO;
uniform int uSSAOEnabled;
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

const float PI = 3.14159265359;

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
    vec3 fresnel = fresnelSchlick(
        max(dot(halfway, viewDir), 0.0),
        f0
    );
    float distribution = distributionGGX(normal, halfway, roughness);
    float geometry = geometrySmith(normal, viewDir, lightDir, roughness);
    float denominator = 4.0 *
                        max(dot(normal, viewDir), 0.0) *
                        max(dot(normal, lightDir), 0.0);
    vec3 specular = distribution * geometry * fresnel /
                    max(denominator, 0.0001);

    vec3 diffuseWeight = (vec3(1.0) - fresnel) * (1.0 - metallic);
    float normalDotLight = max(dot(normal, lightDir), 0.0);
    return (diffuseWeight * albedo / PI + specular) *
           radiance * normalDotLight;
}

float calculateSpecularFactor(
    vec3 normal,
    vec3 lightDir,
    vec3 viewDir,
    float shininess,
    bool useBlinnPhong
)
{
    float specularAngle;
    if (useBlinnPhong) {
        vec3 halfway = lightDir + viewDir;
        float lengthSquared = dot(halfway, halfway);
        if (lengthSquared <= 0.000001) {
            return 0.0;
        }
        halfway *= inversesqrt(lengthSquared);
        specularAngle = max(dot(normal, halfway), 0.0);
    } else {
        vec3 reflectDir = reflect(-lightDir, normal);
        specularAngle = max(dot(reflectDir, viewDir), 0.0);
    }
    return pow(specularAngle, shininess);
}

float calculateDirectionalShadow(
    int lightIndex,
    vec3 fragmentPosition,
    vec3 normal,
    vec3 lightDir
)
{
    if (uShadowsEnabled == 0 || lightIndex != uShadowLightIndex) {
        return 0.0;
    }

    vec4 lightSpacePosition =
            uLightSpaceMatrix * vec4(fragmentPosition, 1.0);
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

float calculatePointShadow(int lightIndex, vec3 fragmentPosition)
{
    if (uPointShadowsEnabled == 0 ||
        lightIndex != uPointShadowLightIndex) {
        return 0.0;
    }

    vec3 fragmentToLight =
            fragmentPosition - uPointShadowLightPosition;
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

    float viewDistance = length(camera.position.xyz - fragmentPosition);
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

vec3 calculateDirectionalLight(
    int lightIndex,
    DirectionalLight light,
    vec3 fragmentPosition,
    vec3 albedo,
    vec3 normal,
    vec3 viewDir,
    float shininess,
    float specularMask,
    bool useBlinnPhong
)
{
    vec3 lightDir = normalize(-light.direction.xyz);
    vec3 color = light.colorIntensity.rgb;
    float intensity = light.colorIntensity.w;
    float diffuseFactor = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = albedo * color * intensity * diffuseFactor;
    vec3 specular = vec3(0.0);
    if (diffuseFactor > 0.0) {
        float factor = calculateSpecularFactor(
            normal,
            lightDir,
            viewDir,
            shininess,
            useBlinnPhong
        );
        specular = color * intensity * factor * specularMask;
    }
    float shadow = calculateDirectionalShadow(
        lightIndex,
        fragmentPosition,
        normal,
        lightDir
    );
    return (diffuse + specular) * (1.0 - shadow);
}

vec3 calculatePointLight(
    int lightIndex,
    PointLight light,
    vec3 fragmentPosition,
    vec3 albedo,
    vec3 normal,
    vec3 viewDir,
    float shininess,
    float specularMask,
    bool useBlinnPhong
)
{
    vec3 toLight = light.positionRange.xyz - fragmentPosition;
    float distance = length(toLight);
    float range = light.positionRange.w;
    if (distance > range) {
        return vec3(0.0);
    }

    vec3 lightDir = toLight / max(distance, 0.0001);
    vec3 color = light.colorIntensity.rgb;
    float intensity = light.colorIntensity.w;
    float diffuseFactor = max(dot(normal, lightDir), 0.0);
    float attenuation = 1.0 - smoothstep(0.0, 1.0, distance / range);
    vec3 diffuse =
            albedo * color * intensity * diffuseFactor * attenuation;
    vec3 specular = vec3(0.0);
    if (diffuseFactor > 0.0) {
        float factor = calculateSpecularFactor(
            normal,
            lightDir,
            viewDir,
            shininess,
            useBlinnPhong
        );
        specular = color * intensity * attenuation * factor * specularMask;
    }
    float shadow = calculatePointShadow(lightIndex, fragmentPosition);
    return (diffuse + specular) * (1.0 - shadow);
}

vec3 calculateSpotLight(
    SpotLight light,
    vec3 fragmentPosition,
    vec3 albedo,
    vec3 normal,
    vec3 viewDir,
    float shininess,
    float specularMask,
    bool useBlinnPhong
)
{
    vec3 toLight = light.positionRange.xyz - fragmentPosition;
    float distance = length(toLight);
    float range = light.positionRange.w;
    if (distance > range) {
        return vec3(0.0);
    }

    vec3 lightDir = toLight / max(distance, 0.0001);
    float coneFactor = smoothstep(
        light.coneAngles.y,
        light.coneAngles.x,
        dot(-lightDir, normalize(light.direction.xyz))
    );
    if (coneFactor <= 0.0) {
        return vec3(0.0);
    }

    float attenuation = 1.0 - smoothstep(0.0, 1.0, distance / range);
    vec3 color = light.colorIntensity.rgb;
    float intensity = light.colorIntensity.w;
    float diffuseFactor = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = albedo * color * intensity * diffuseFactor *
                   attenuation * coneFactor;
    vec3 specular = vec3(0.0);
    if (diffuseFactor > 0.0) {
        float factor = calculateSpecularFactor(
            normal,
            lightDir,
            viewDir,
            shininess,
            useBlinnPhong
        );
        specular = color * intensity * coneFactor * factor * specularMask;
    }
    return diffuse + specular;
}

void main()
{
    vec4 positionSample = texture(gPosition, vTexCoord);
    if (positionSample.a <= 0.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec4 normalSample = texture(gNormal, vTexCoord);
    vec4 albedoSpecSample = texture(gAlbedoSpec, vTexCoord);
    vec4 materialSample = texture(gMaterial, vTexCoord);
    vec3 fragmentPosition = positionSample.xyz;
    vec3 normal = normalize(normalSample.xyz);
    vec3 albedo = albedoSpecSample.rgb;
    float specularMask = albedoSpecSample.a;
    float shininess = max(normalSample.a, 0.0001);
    bool useBlinnPhong = positionSample.a > 1.5;
    bool usePBR = materialSample.r > 0.5;
    float metallic = clamp(materialSample.g, 0.0, 1.0);
    float roughness = clamp(materialSample.b, 0.04, 1.0);
    float materialAO = clamp(materialSample.a, 0.0, 1.0);

    if (debugData.viewMode == 1) {
        float depth = -(camera.view * vec4(fragmentPosition, 1.0)).z;
        float range = max(
            debugData.depthFar - debugData.depthNear,
            0.0001
        );
        float gray = clamp(
            (depth - debugData.depthNear) / range,
            0.0,
            1.0
        );
        FragColor = vec4(vec3(gray), 1.0);
        return;
    }
    if (debugData.viewMode == 2) {
        FragColor = vec4(normal * 0.5 + 0.5, 1.0);
        return;
    }
    if (debugData.viewMode == 3) {
        FragColor = vec4(
            clamp(fragmentPosition * 0.05 + 0.5, 0.0, 1.0),
            1.0
        );
        return;
    }
    if (debugData.viewMode == 4) {
        FragColor = vec4(albedo, 1.0);
        return;
    }
    if (debugData.viewMode == 5) {
        FragColor = vec4(vec3(specularMask), 1.0);
        return;
    }

    float ambientOcclusion = uSSAOEnabled != 0
        ? texture(uSSAO, vTexCoord).r
        : 1.0;
    if (debugData.viewMode == 6) {
        FragColor = vec4(vec3(ambientOcclusion), 1.0);
        return;
    }

    vec3 result = albedo *
                  lights.ambientLightColorIntensity.rgb *
                  lights.ambientLightColorIntensity.w *
                  ambientOcclusion *
                  (usePBR ? materialAO : 1.0);
    vec3 viewDir = normalize(camera.position.xyz - fragmentPosition);
    int directionalCount = min(
        lights.lightCounts.x,
        MAX_DIRECTIONAL_LIGHTS
    );
    int pointCount = min(lights.lightCounts.y, MAX_POINT_LIGHTS);
    int spotCount = min(lights.lightCounts.z, MAX_SPOT_LIGHTS);

    for (int index = 0; index < directionalCount; ++index) {
        if (usePBR) {
            DirectionalLight light = lights.directionalLights[index];
            vec3 lightDir = normalize(-light.direction.xyz);
            vec3 radiance = light.colorIntensity.rgb *
                            light.colorIntensity.w;
            float shadow = calculateDirectionalShadow(
                index,
                fragmentPosition,
                normal,
                lightDir
            );
            result += evaluatePBR(
                albedo,
                metallic,
                roughness,
                normal,
                viewDir,
                lightDir,
                radiance
            ) * (1.0 - shadow);
        } else {
            result += calculateDirectionalLight(
                index,
                lights.directionalLights[index],
                fragmentPosition,
                albedo,
                normal,
                viewDir,
                shininess,
                specularMask,
                useBlinnPhong
            );
        }
    }
    for (int index = 0; index < pointCount; ++index) {
        if (usePBR) {
            PointLight light = lights.pointLights[index];
            vec3 toLight = light.positionRange.xyz - fragmentPosition;
            float distance = length(toLight);
            if (distance <= light.positionRange.w) {
                vec3 lightDir = toLight / max(distance, 0.0001);
                float rangeFade = 1.0 - smoothstep(
                    0.0,
                    1.0,
                    distance / light.positionRange.w
                );
                vec3 radiance = light.colorIntensity.rgb *
                                light.colorIntensity.w * rangeFade;
                float shadow = calculatePointShadow(index, fragmentPosition);
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
        } else {
            result += calculatePointLight(
                index,
                lights.pointLights[index],
                fragmentPosition,
                albedo,
                normal,
                viewDir,
                shininess,
                specularMask,
                useBlinnPhong
            );
        }
    }
    for (int index = 0; index < spotCount; ++index) {
        if (usePBR) {
            SpotLight light = lights.spotLights[index];
            vec3 toLight = light.positionRange.xyz - fragmentPosition;
            float distance = length(toLight);
            if (distance <= light.positionRange.w) {
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
                                light.colorIntensity.w * coneFactor *
                                rangeFade;
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
        } else {
            result += calculateSpotLight(
                lights.spotLights[index],
                fragmentPosition,
                albedo,
                normal,
                viewDir,
                shininess,
                specularMask,
                useBlinnPhong
            );
        }
    }

    float distanceToCamera = length(
        camera.position.xyz - fragmentPosition
    );
    float fogAmount = smoothstep(40.0, 100.0, distanceToCamera);
    vec3 finalColor = mix(result, vec3(0.5, 0.6, 0.7), fogAmount);
    FragColor = vec4(finalColor, 1.0);
}
