#version 330 core

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

layout(location = 0) in uint posIndex;

out float brightness;

void main() {
	gl_Position.x = mod(posIndex, 33u);
	gl_Position.y = mod(posIndex / 33u, 33u);
	gl_Position.z = posIndex / (33u * 33u);
    gl_Position.w = 1.0;
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * gl_Position;
    brightness = 10.0 / length(gl_Position.xyz);
 }