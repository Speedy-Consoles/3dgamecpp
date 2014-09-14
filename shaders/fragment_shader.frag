#version 120

uniform sampler2D colorMap;
uniform vec3 fog_color;
uniform float fog_end;
uniform float fog_width;
varying vec3 fNormal;
varying vec3 fPosition;
varying vec4 fColor;

void main() {
	float fogFac = clamp((length(fPosition) - fog_end + fog_width) / fog_width, 0.0, 1.0);
	vec4 color = fColor * texture2D( colorMap, vec2(gl_TexCoord[0]));
	vec3 combColor3 = mix(vec3(color), vec3(fog_color), fogFac);
	gl_FragColor = vec4(combColor3[0], combColor3[1], combColor3[2], color[3]);
}