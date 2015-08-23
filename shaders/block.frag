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
uniform float fogStartDistance;
uniform float fogEndDistance;

uniform sampler2DArray textureSampler;
uniform sampler2D fogSampler;

in vec3 vfNormal;
in vec2 vfTexturePosition;
in vec2 vfCornerPosition;
in vec3 vfRealPosition;
in float[4] vfShadowLevels;
flat in uint vfTextureIndex;

out vec4 fColor;
 
void main() {
	vec2 cp = clamp(vfCornerPosition, 0.0, 1.0);
	float shadowLevel = vfShadowLevels[0] * (1 - cp.x) * (1 - cp.y)
						+ vfShadowLevels[1] * cp.x * (1 - cp.y)
						+ vfShadowLevels[3] * (1 - cp.x) * cp.y
						+ vfShadowLevels[2] * cp.x * cp.y;
	vec4 texColor = texture(textureSampler, vec3(vfTexturePosition, vfTextureIndex));

	vec4 sceneColor = texColor * vec4(vec3(1.0 - shadowLevel / 2.0), 1.0);
	if (lightEnabled) {
		vec3 diffuseLight = max(0, dot(vfNormal, normalize(diffuseLightDirection))) * diffuseLightColor;
		sceneColor.xyz = sceneColor.xyz * (ambientLightColor + diffuseLight);
	}
	if (fogEnabled) {
		float fogAlpha = clamp((length(vfRealPosition) - fogStartDistance) / (fogEndDistance - fogStartDistance), 0.0, 1.0);
		vec3 fogPart = fogAlpha * texelFetch(fogSampler, ivec2(gl_FragCoord.xy), 0).xyz;
		vec3 scenePart = sceneColor.xyz * (1.0 - fogAlpha);
		fColor = vec4(scenePart + fogPart, sceneColor.a);
	} else {
		fColor = sceneColor;
	}
}