#version 330 core

uniform mat4 projectionMatrix;

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;

out vec4 vfColor;

void main() {
	gl_Position.xy = position;
	gl_Position.z = -1.0;
	gl_Position.w = 1.0;
	gl_Position = projectionMatrix * gl_Position;
	vfColor = color;
}