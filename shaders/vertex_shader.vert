#version 330 core

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

layout(location = 0) in vec3 pos;

void main() {
	gl_Position.xyz = pos;
    gl_Position.w = 1.0;
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * gl_Position;
 }