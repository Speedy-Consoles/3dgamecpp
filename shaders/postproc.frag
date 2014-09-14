uniform float fxaa_span_max;
uniform float fxaa_reduce_mul;
uniform float fxaa_reduce_min;

uniform sampler2D fbo_texture;
uniform vec2 pixel_size;
varying vec2 f_texcoord;

void main(void) {
	vec3 rgbNW = texture2D(fbo_texture, f_texcoord + (vec2(-1.0, -1.0) * pixel_size)).rgb;
	vec3 rgbNE = texture2D(fbo_texture, f_texcoord + (vec2(+1.0, -1.0) * pixel_size)).rgb;
	vec3 rgbSW = texture2D(fbo_texture, f_texcoord + (vec2(-1.0, +1.0) * pixel_size)).rgb;
	vec3 rgbSE = texture2D(fbo_texture, f_texcoord + (vec2(+1.0, +1.0) * pixel_size)).rgb;
	vec3 rgbM  = texture2D(fbo_texture, f_texcoord).rgb;
	
	vec3 luma = vec3(0.299, 0.587, 0.114);
	float lumaNW = dot(rgbNW, luma);
	float lumaNE = dot(rgbNE, luma);
	float lumaSW = dot(rgbSW, luma);
	float lumaSE = dot(rgbSE, luma);
	float lumaM  = dot( rgbM, luma);
	
	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

	vec2 dir;
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));
	
	float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * fxaa_reduce_mul), fxaa_reduce_min);
	
	float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
	
	dir = min(vec2(fxaa_span_max, fxaa_span_max),
			max(vec2(-fxaa_span_max, -fxaa_span_max), dir * rcpDirMin)) * pixel_size;
	
	vec3 rgbA = (1.0/2.0) * (
	texture2D(fbo_texture, f_texcoord + dir * (1.0/3.0 - 0.5)).rgb +
	texture2D(fbo_texture, f_texcoord + dir * (2.0/3.0 - 0.5)).rgb);
	vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
	texture2D(fbo_texture, f_texcoord + dir * (0.0/3.0 - 0.5)).rgb +
	texture2D(fbo_texture, f_texcoord + dir * (3.0/3.0 - 0.5)).rgb);
	float lumaB = dot(rgbB, luma);
	
	if((lumaB < lumaMin) || (lumaB > lumaMax)) {
		gl_FragColor.rgb=rgbA;
	} else {
		gl_FragColor.rgb=rgbB;
	}

	gl_FragColor.a = 1.0;
}