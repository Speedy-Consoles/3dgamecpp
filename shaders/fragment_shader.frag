uniform sampler2D colorMap;
uniform vec3 fog_color;
uniform float fog_start;
uniform float fog_end;
varying vec4 gl_Color;

void main() {
	vec3 v = vec3(gl_FragCoord) / gl_FragCoord.w;
	float fogFac = clamp((gl_DepthRange.near + v.z - fog_start) / fog_end, 0.0, 1.0);
	vec4 color = gl_Color * texture2D( colorMap, gl_TexCoord[0].st);
	vec3 combColor3 = mix(vec3(color), vec3(fog_color), fogFac);
	gl_FragColor = vec4(combColor3[0], combColor3[1], combColor3[2], color[3]);
}