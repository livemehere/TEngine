#version 410 core

in vec3 vWorldPosition;

uniform vec3 uLightPosition;
uniform float uFarPlane;

void main()
{
    float lightDistance = length(vWorldPosition - uLightPosition);
    gl_FragDepth = lightDistance / uFarPlane;
}
