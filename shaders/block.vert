#version 330 core

//in int gl_VertexID;
//in int gl_InstanceID;

//out gl_PerVertex {
//	vec4 gl_Position;
//	float gl_PointSize;
//	float gl_ClipDistance[];
//};

const vec3 NORMALS[6] = vec3[6](
	vec3( 1.0,  0.0,  0.0),
	vec3( 0.0,  1.0,  0.0),
	vec3( 1.0,  0.0,  1.0),
	vec3(-1.0,  0.0,  0.0),
	vec3( 0.0, -1.0,  0.0),
	vec3( 0.0,  0.0, -1.0)
);

const vec2 CORNER_POSITIONS[4] = vec2[4](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(1.0, 1.0),
	vec2(0.0, 1.0)
);

const vec2 TEXTURE_POSITIONS[4] = vec2[4](
	vec2(0.0, 1.0),
	vec2(1.0, 1.0),
	vec2(1.0, 0.0),
	vec2(0.0, 0.0)
);

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

layout(location = 0) in uint posIndex;
layout(location = 1) in uint textureIndex;
layout(location = 2) in uint dirIndexCornerIndex;
layout(location = 3) in uint shadowLevels;

out vec3 vfNormal;
flat out uint vfTextureIndex;
out vec2 vfTexturePosition;
out vec2 vfCornerPosition;
out vec3 vfRealPosition;
out float[4] vfShadowLevels;

void main() {
	vec4 position = vec4(mod(posIndex, 33u), mod(posIndex / 33u, 33u), posIndex / (33u * 33u), 1.0);
	vec4 realPosition = viewMatrix * modelMatrix * position;
	vfRealPosition = realPosition.xyz;
	gl_Position = projectionMatrix * realPosition;
	
	vfTextureIndex = textureIndex;
	
	int dirIndex = int(dirIndexCornerIndex & 7u);
	vfNormal = NORMALS[dirIndex];

	for (int i = 0; i < 4; i++) {
		vfShadowLevels[i] = float((shadowLevels >> 2 * i) & 3u) / 3.0;
	}

	uint cornerIndex = dirIndexCornerIndex >> 3u;
	vfCornerPosition = CORNER_POSITIONS[cornerIndex];
	vfTexturePosition = TEXTURE_POSITIONS[cornerIndex];
}