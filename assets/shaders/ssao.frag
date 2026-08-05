#version 410 core

layout (std140) uniform CameraData {
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

in vec2 vTexCoord;
out float FragColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D uNoiseTexture;
uniform vec3 uSamples[64];
uniform int uSampleCount;
uniform float uRadius;
uniform float uBias;
uniform float uPower;
uniform float uResolutionScale;

void main()
{
    vec4 worldPositionSample = texture(gPosition, vTexCoord);
    if (worldPositionSample.a <= 0.0) {
        FragColor = 1.0;
        return;
    }

    vec3 fragmentPosition = vec3(
        camera.view * vec4(worldPositionSample.xyz, 1.0)
    );
    vec3 worldNormal = normalize(texture(gNormal, vTexCoord).xyz);
    vec3 normal = normalize(mat3(camera.view) * worldNormal);

    vec2 noiseScale =
            vec2(textureSize(gPosition, 0)) * uResolutionScale /
            vec2(textureSize(uNoiseTexture, 0));
    vec3 randomVector = normalize(
        texture(uNoiseTexture, vTexCoord * noiseScale).xyz
    );
    vec3 tangent = randomVector - normal * dot(randomVector, normal);
    float tangentLengthSquared = dot(tangent, tangent);
    if (tangentLengthSquared <= 0.000001) {
        tangent = abs(normal.z) < 0.999
            ? normalize(cross(vec3(0.0, 0.0, 1.0), normal))
            : normalize(cross(vec3(0.0, 1.0, 0.0), normal));
    } else {
        tangent *= inversesqrt(tangentLengthSquared);
    }
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);

    int sampleCount = clamp(uSampleCount, 1, 64);
    float radius = max(uRadius, 0.001);
    float occlusion = 0.0;
    for (int index = 0; index < 64; ++index) {
        if (index >= sampleCount) {
            break;
        }

        vec3 samplePosition =
                fragmentPosition + tbn * uSamples[index] * radius;
        if (samplePosition.z >= -0.0001) {
            continue;
        }
        vec4 offset = camera.projection * vec4(samplePosition, 1.0);
        if (abs(offset.w) <= 0.0001) {
            continue;
        }
        offset.xyz /= offset.w;
        vec2 sampleUv = offset.xy * 0.5 + 0.5;
        if (sampleUv.x < 0.0 || sampleUv.x > 1.0 ||
            sampleUv.y < 0.0 || sampleUv.y > 1.0) {
            continue;
        }

        vec4 sampledWorldPosition = texture(gPosition, sampleUv);
        if (sampledWorldPosition.a <= 0.0) {
            continue;
        }
        float sampleDepth = (
            camera.view * vec4(sampledWorldPosition.xyz, 1.0)
        ).z;
        float depthDifference = abs(fragmentPosition.z - sampleDepth);
        float rangeCheck = smoothstep(
            0.0,
            1.0,
            radius / max(depthDifference, 0.0001)
        );
        occlusion += sampleDepth >= samplePosition.z + uBias
            ? rangeCheck
            : 0.0;
    }

    float accessibility = 1.0 - occlusion / float(sampleCount);
    FragColor = pow(clamp(accessibility, 0.0, 1.0), max(uPower, 0.01));
}
