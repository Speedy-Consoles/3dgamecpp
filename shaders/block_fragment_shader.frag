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

uniform float fogDistance;

uniform sampler2DArray sampler;

in vec3 vfNormal;
in vec2 vfTexturePosition;
in vec2 vfCornerPosition;
in vec3 vfRealPosition;
in float[4] vfShadowLevels;
flat in uint vfTextureIndex;

layout(location = 0) out vec4 fColor;
layout(location = 1) out vec4 fDistance;
 
void main() {
	float shadowLevel = vfShadowLevels[0] * (1 - vfCornerPosition.x) * (1 - vfCornerPosition.y)
						+ vfShadowLevels[1] * vfCornerPosition.x * (1 - vfCornerPosition.y)
						+ vfShadowLevels[3] * (1 - vfCornerPosition.x) * vfCornerPosition.y
						+ vfShadowLevels[2] * vfCornerPosition.x * vfCornerPosition.y;
	vec4 texColor = texture(sampler, vec3(vfTexturePosition, vfTextureIndex));

	if (lightEnabled) {
		vec3 diffuseLight = max(0, dot(vfNormal, normalize(diffuseLightDirection))) * diffuseLightColor;
		fColor = texColor * vec4((ambientLightColor + diffuseLight) * (1.0 - shadowLevel / 2.0), 1.0);
	} else {
		fColor = texColor * vec4(vec3(1.0 - shadowLevel / 2.0), 1.0);
	}

	// TODO why this?
	fDistance = vec4(length(vfRealPosition) / fogDistance, 0.0, 0.0, 1.0);
}