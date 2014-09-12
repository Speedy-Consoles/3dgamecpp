uniform sampler2D colorMap;

void main() {
	float d = 1.0 - clamp(gl_FragCoord[2] / gl_FragCoord[3] / 100.0, 0.0, 1.0);
	gl_FragColor = gl_FragColor = gl_FrontColor * texture2D( colorMap, gl_TexCoord[0].st);
}