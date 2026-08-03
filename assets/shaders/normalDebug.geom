#version 410 core

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout (std140) uniform CameraData {
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

in VS_OUT {
    vec3 worldNormal;
} geometryIn[];

uniform float uNormalLength;

void generateNormalLine(int index)
{
    vec4 worldPosition = gl_in[index].gl_Position;
    vec3 worldNormal = normalize(geometryIn[index].worldNormal);

    gl_Position = camera.projection * camera.view * worldPosition;
    EmitVertex();

    gl_Position = camera.projection * camera.view * (
        worldPosition + vec4(worldNormal * uNormalLength, 0.0)
    );
    EmitVertex();

    EndPrimitive();
}

void main()
{
    generateNormalLine(0);
    generateNormalLine(1);
    generateNormalLine(2);
}
