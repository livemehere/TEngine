#version 410 core

layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

layout (std140) uniform CameraData {
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

void main()
{
    TexCoords = aPos;
    mat4 rotationView = mat4(mat3(camera.view));
    vec4 position = camera.projection * rotationView * vec4(aPos, 1.0);
    gl_Position = position.xyww;
}
