#version 410 core

layout (std140) uniform CameraData {
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

layout (std140) uniform DebugData {
    int viewMode;
    float depthNear;
    float depthFar;
    int padding;
} debugData;

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vPos;

out vec4 FragColor;

uniform sampler2D uAlbedoTexture;
uniform vec4 uBaseColor;

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

void main()
{
    vec4 color = texture(uAlbedoTexture, vTexCoord) * uBaseColor;
    FragColor = applyDebugView(color);
}
