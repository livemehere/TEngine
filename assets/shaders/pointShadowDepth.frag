#version 410 core

in vec3 gWorldPosition;

uniform vec3 uLightPosition;
uniform float uFarPlane;

void main()
{
    float lightDistance = length(gWorldPosition - uLightPosition);
    gl_FragDepth = lightDistance / uFarPlane;
}
