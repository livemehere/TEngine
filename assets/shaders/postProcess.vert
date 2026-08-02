#version 410 core

layout (std140) uniform CameraData {
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

out vec2 vTexCoord;

const vec2 positions[3] = vec2[](
vec2(-1.0, -1.0),
vec2(3.0, -1.0),
vec2(-1.0, 3.0)
);

const vec2 texCoords[3] = vec2[](
vec2(0.0, 0.0),
vec2(2.0, 0.0),
vec2(0.0, 2.0)
);

void main()
{
    gl_Position = vec4(
    positions[gl_VertexID],
    0.0,
    1.0
    );

    vTexCoord = texCoords[gl_VertexID];
}