uniform sampler2D fbo_texture;
varying vec2 f_texcoord;
 
void main(void) {
	vec3 luma_vec = vec3(0.299, 0.587, 0.114);
	vec4 tex_col = texture2D(fbo_texture, f_texcoord);
	float luma = dot(tex_col.rgb, luma_vec);
	gl_FragColor = vec4(luma, luma, luma, tex_col.a);
}