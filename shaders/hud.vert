#version 330 core

//in int gl_VertexID;
//in int gl_InstanceID;

//out gl_PerVertex {
//	vec4 gl_Position;
//	float gl_PointSize;
//	float gl_ClipDistance[];
//};

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