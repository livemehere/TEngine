#version 410 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uTexture2D;
uniform samplerCube uCubeTexture;
uniform int uMode;
uniform int uCubeFace;
uniform int uOrthographic;
uniform float uCameraNear;
uniform float uCameraFar;
uniform float uDepthRangeNear;
uniform float uDepthRangeFar;

float linearizeDepth(float depth)
{
    if (uOrthographic != 0) {
        return mix(uCameraNear, uCameraFar, depth);
    }

    float ndcDepth = depth * 2.0 - 1.0;
    return (2.0 * uCameraNear * uCameraFar) /
           (uCameraFar + uCameraNear -
            ndcDepth * (uCameraFar - uCameraNear));
}

vec3 cubeDirection(vec2 texCoords, int face)
{
    vec2 position = texCoords * 2.0 - 1.0;
    position.y = -position.y;

    if (face == 0) return normalize(vec3(1.0, position.y, -position.x));
    if (face == 1) return normalize(vec3(-1.0, position.y, position.x));
    if (face == 2) return normalize(vec3(position.x, 1.0, -position.y));
    if (face == 3) return normalize(vec3(position.x, -1.0, position.y));
    if (face == 4) return normalize(vec3(position.x, position.y, 1.0));
    return normalize(vec3(-position.x, position.y, -1.0));
}

void main()
{
    vec4 sampleValue = texture(uTexture2D, vTexCoord);

    if (uMode == 1) {
        vec3 position = clamp(sampleValue.xyz * 0.05 + 0.5, 0.0, 1.0);
        FragColor = vec4(sampleValue.a == 0.0 ? vec3(0.0) : position, 1.0);
        return;
    }
    if (uMode == 2) {
        if (sampleValue.a == 0.0) {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }
        vec3 normal = normalize(sampleValue.xyz) * 0.5 + 0.5;
        FragColor = vec4(normal, 1.0);
        return;
    }
    if (uMode == 3) {
        FragColor = vec4(sampleValue.rgb, 1.0);
        return;
    }
    if (uMode == 4) {
        FragColor = vec4(vec3(sampleValue.a), 1.0);
        return;
    }
    if (uMode == 5) {
        float linearDepth = linearizeDepth(sampleValue.r);
        float range = max(uDepthRangeFar - uDepthRangeNear, 0.0001);
        float gray = clamp(
            (linearDepth - uDepthRangeNear) / range,
            0.0,
            1.0
        );
        FragColor = vec4(vec3(gray), 1.0);
        return;
    }
    if (uMode == 6 || uMode == 7) {
        FragColor = vec4(vec3(sampleValue.r), 1.0);
        return;
    }
    if (uMode == 8) {
        FragColor = vec4(vec3(pow(sampleValue.r, 24.0)), 1.0);
        return;
    }
    if (uMode == 9) {
        float depth = texture(
            uCubeTexture,
            cubeDirection(vTexCoord, uCubeFace)
        ).r;
        FragColor = vec4(vec3(depth), 1.0);
        return;
    }

    FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
