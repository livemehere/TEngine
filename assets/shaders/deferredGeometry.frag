#version 410 core

layout (std140) uniform CameraData {
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vPos;
in mat3 vTBN;

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

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
};

uniform Material material;

vec2 getParallaxTexCoords(vec2 texCoords, vec3 tangentViewDir)
{
    if (material.hasDepthMap == 0 || material.parallaxMode == 0) {
        return texCoords;
    }

    float viewDepth = max(tangentViewDir.z, 0.05);
    if (material.parallaxMode == 1) {
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
    vec2 deltaTexCoords =
            tangentViewDir.xy * material.parallaxScale / layerCount;
    vec2 currentTexCoords = texCoords;
    float currentLayerDepth = 0.0;
    float currentMapDepth = texture(
        material.depthMap,
        currentTexCoords
    ).r;

    for (int layer = 0; layer < 64; ++layer) {
        if (currentLayerDepth >= currentMapDepth ||
            float(layer) >= layerCount) {
            break;
        }
        currentTexCoords -= deltaTexCoords;
        currentMapDepth = texture(
            material.depthMap,
            currentTexCoords
        ).r;
        currentLayerDepth += layerDepth;
    }

    if (material.parallaxMode == 2) {
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
    if (material.hasNormalMap == 0) {
        return normalize(vNormal);
    }

    vec3 tangentNormal = texture(material.normalMap, texCoords).rgb;
    tangentNormal = tangentNormal * 2.0 - 1.0;
    tangentNormal.xy *= max(material.normalStrength, 0.0);
    if (material.flipNormalY != 0) {
        tangentNormal.y = -tangentNormal.y;
    }
    return normalize(vTBN * normalize(tangentNormal));
}

void main()
{
    vec3 viewDir = normalize(camera.position.xyz - vPos);
    vec3 tangentViewDir = normalize(transpose(vTBN) * viewDir);
    vec2 texCoords = getParallaxTexCoords(vTexCoord, tangentViewDir);

    if (material.hasDepthMap != 0 &&
        material.discardParallaxEdges != 0 &&
        (texCoords.x < 0.0 || texCoords.x > 1.0 ||
         texCoords.y < 0.0 || texCoords.y > 1.0)) {
        discard;
    }

    vec3 albedo = texture(material.albedo, texCoords).rgb *
                  material.baseColor.rgb;
    float specular = material.hasSpecularMap != 0
        ? texture(material.specular, texCoords).r *
          material.specularStrength
        : 0.0;

    gPosition = vec4(
        vPos,
        material.useBlinnPhong != 0 ? 2.0 : 1.0
    );
    gNormal = vec4(
        getSurfaceNormal(texCoords),
        max(material.shininess, 0.0001)
    );
    gAlbedoSpec = vec4(albedo, max(specular, 0.0));
}
