#version 120

uniform mat4 P;
uniform mat4 MV;
uniform mat4 itMV;

attribute vec4 aPos; // in object space
attribute vec3 aNor; // in object space

varying vec3 normal;
varying vec4 position;


void main()
{
	gl_Position = P * MV * aPos;
    position = MV * aPos;
    normal = normalize(mat3(itMV) * aNor);

}
