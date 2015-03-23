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

uniform sampler2D sceneColorSampler;
uniform sampler2D sceneDistanceSampler;
uniform sampler2D sceneDepthSampler;

in vec4 vfColor;

out vec4 fColor;
 
void main() {
	float sceneDepth = texelFetch(sceneDepthSampler, ivec2(gl_FragCoord.xy), 0);
	if (sceneDepth == 1.0) {
		fColor = vfColor;
		gl_FragDepth = 1.0;
	} else {
		vec4 sceneColor = texelFetch(sceneColorSampler, ivec2(gl_FragCoord.xy), 0);
		float sceneDistance = texelFetch(sceneDistanceSampler, ivec2(gl_FragCoord.xy), 0);
		float sceneAlpha = sceneColor.a * (1.0 - sceneDistance);
		vec3 scenePart = sceneAlpha * sceneColor.rgb;
		vec3 skyPart = (1.0 - sceneAlpha) * vfColor.rgb;
		fColor = vec4(scenePart + skyPart, 1.0);
	}
	gl_FragDepth = sceneDepth;
}