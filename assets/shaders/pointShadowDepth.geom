#version 410 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 uShadowMatrices[6];

out vec3 gWorldPosition;

void main()
{
    for(int face = 0; face < 6; ++face){
        gl_Layer = face;
        for(int vertex = 0; vertex < 3; ++vertex){
            gWorldPosition = gl_in[vertex].gl_Position.xyz;
            gl_Position = uShadowMatrices[face] * gl_in[vertex].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
}
