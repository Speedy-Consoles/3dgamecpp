#version 330 core

//struct gl_DepthRangeParameters {
//	float near;
//	float far;
//	float diff;
//};
//uniform gl_DepthRangeParameters gl_DepthRange;

//in vec4 gl_FragCoord;
//in bool gl_FrontFacing;
//in vec2 gl_PointCoord;

// out float gl_FragDepth;

uniform vec3 ambientLightColor;
uniform vec3 diffuseLightDirection;
uniform vec3 diffuseLightColor;

in vec3 vfNormal;
in vec4 vfColor;

out vec4 fColor;
 
void main() {
    vec3 diffuseLight = max(0, dot(vfNormal, normalize(diffuseLightDirection))) * diffuseLightColor;
    fColor = vec4(ambientLightColor + diffuseLight, 1.0) * vfColor;
}