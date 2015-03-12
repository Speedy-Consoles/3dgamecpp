#version 330 core

const vec3 NORMALS[6] = vec3[6](
		vec3( 1.0,  0.0,  0.0),
		vec3( 0.0,  1.0,  0.0),
		vec3( 1.0,  0.0,  1.0),
		vec3(-1.0,  0.0,  0.0),
		vec3( 0.0, -1.0,  0.0),
		vec3( 0.0,  0.0, -1.0)
);

uniform vec3 diffuseLightDirection;
uniform vec3 diffuseLightColor;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

layout(location = 0) in uint posIndex;
layout(location = 1) in uint dirIndexShadowLevel;

out float brightness;

void main() {
	gl_Position.x = mod(posIndex, 33u);
	gl_Position.y = mod(posIndex / 33u, 33u);
	gl_Position.z = posIndex / (33u * 33u);
    gl_Position.w = 1.0;
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * gl_Position;
    uint dirIndex = dirIndexShadowLevel & 7u;
    uint shadowLevel = dirIndexShadowLevel >> 3u;
    brightness = 1.0 - float(shadowLevel) / 3.0 * 0.2;
    brightness = brightness / (float(dirIndex) + 1.0);
 }