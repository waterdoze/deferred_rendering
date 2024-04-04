#version 120

uniform mat4 P;
uniform mat4 MV;
uniform mat4 itMV;
uniform float t;

attribute vec4 aPos; // In object space
attribute vec3 aNor; // In object space
attribute vec2 aTex;

varying vec3 normal;
varying vec4 position;

void main()
{
    float f = cos(aPos.x + t) + 2;
    float fprime = -sin(aPos.x + t);
    float y = f * cos(aPos.y);
    float z = f * sin(aPos.y);

    vec3 dpdx = vec3(1.0f, fprime * cos(aPos.y), fprime * sin(aPos.y));
    vec3 dpdtheta = vec3(0, -f * sin(aPos.y), f * cos(aPos.y));
    vec3 normalv = cross(dpdx, dpdtheta);
    vec3 normalizedNormalV =  - normalize(normalv);

    vec4 aPosNew = vec4(aPos.x, y, z, 1.0);
    vec3 aNorNew = vec3(normalizedNormalV.x, normalizedNormalV.y, normalizedNormalV.z);

	gl_Position = P * (MV * aPosNew);
    position = MV * aPosNew;
    normal = normalize(mat3(itMV) * aNorNew);
}
