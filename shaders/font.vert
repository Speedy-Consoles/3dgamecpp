#version 330 core

uniform mat4 projectionMatrix;
uniform mat4 modelMatrix;

layout(location = 0) in vec2 coord;
layout(location = 1) in vec2 texCoord;
//layout(location = 2) in vec4 color;

out vec4 vfColor;
out vec2 vfTexCoord;

void main() {
	vec4 fullCoord;
	fullCoord.xy = coord;
	fullCoord.z = -1.0;
	fullCoord.w = 1.0;
	gl_Position = projectionMatrix * fullCoord;
	vfTexCoord = texCoord;
	vfColor = vec4(1.0);
}