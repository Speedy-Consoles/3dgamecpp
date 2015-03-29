#version 330 core

uniform mat4 transformMatrix;

layout(location = 0) in vec2 coord;
layout(location = 1) in vec2 texCoord;

out vec2 vfTexCoord;

void main() {
	vec4 fullCoord;
	fullCoord.xy = coord;
	fullCoord.z = -1.0;
	fullCoord.w = 1.0;
	gl_Position = transformMatrix * fullCoord;
	vfTexCoord = texCoord;
}