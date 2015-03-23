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

uniform bool lightEnabled;
uniform vec3 ambientLightColor;
uniform vec3 diffuseLightDirection;
uniform vec3 diffuseLightColor;

uniform bool fogEnabled;
uniform float fogDistance;

uniform sampler2D textureSampler;
uniform sampler2D fogSampler;

in vec3 vfNormal;
in vec4 vfColor;
in vec3 vfRealPosition;
in vec2 vfTexturePosition;

out vec4 fColor;
 
void main() {
	vec4 sceneColor = vfColor * vec4(1.0);//texture(textureSampler, vfTexturePosition);
	if (lightEnabled) {
		vec3 diffuseLight = max(0, dot(vfNormal, normalize(diffuseLightDirection))) * diffuseLightColor;
		sceneColor.xyz = sceneColor.xyz * (ambientLightColor + diffuseLight);
	}
	if (fogEnabled) {
		float fogAlpha = clamp(length(vfRealPosition) / fogDistance, 0.0, 1.0);
		vec3 fogPart = fogAlpha * texelFetch(fogSampler, ivec2(gl_FragCoord.xy), 0).xyz;
		vec3 scenePart = sceneColor.xyz * (1.0 - fogAlpha);
		fColor = vec4(scenePart + fogPart, sceneColor.a);
	} else {
		fColor = sceneColor;
	}
}