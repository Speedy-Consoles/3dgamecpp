#version 330 core

//in int gl_VertexID;
//in int gl_InstanceID;

//out gl_PerVertex {
//  vec4 gl_Position;
//  float gl_PointSize;
//  float gl_ClipDistance[];
//};

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;

out vec4 vfColor;
out vec3 vfRealPosition;
out vec3 vfNormal;
out vec2 vfTexturePosition;

void main() {
	vfColor = color;
	vec4 realPosition = viewMatrix * modelMatrix * vec4(position, 1.0);
	vfRealPosition = realPosition.xyz;
	gl_Position = projectionMatrix * realPosition;
	vfNormal = normal * inverse(mat3(modelMatrix));
	vfTexturePosition = vec2(0.0);
}