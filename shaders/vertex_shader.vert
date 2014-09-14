#version 120

attribute vec3 gl_Vertex;
attribute vec3 gl_Normal;
attribute vec4 gl_Color;
uniform mat3 gl_NormalMatrix;
uniform mat4 gl_ModelViewMatrix;
uniform mat4 gl_ProjectionMatrix;
varying vec3 fNormal;
varying vec3 fPosition;
varying vec2 fTexPos;
varying vec4 fColor;

void main() {
	// calculate normal
	fNormal = normalize(gl_NormalMatrix * gl_Normal);

	// calculate position
	vec4 pos = gl_ModelViewMatrix * vec4(gl_Vertex, 1.0);
	fPosition = vec3(pos);
	gl_Position = gl_ProjectionMatrix * pos;

	// calculate color
	vec3 lightDir = normalize(vec3(gl_LightSource[0].position));
	float NdotL = max(dot(fNormal, lightDir), 0.0);
	vec4 diffuse = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse;
	vec4 ambient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;
	fColor =  gl_Color * (NdotL * diffuse + ambient);

	// set texture coords
	gl_TexCoord[0] = gl_MultiTexCoord0;
}