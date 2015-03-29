#version 330 core

uniform sampler2DArray tex;
uniform bool isPacked;
uniform bool hasOutline;
uniform int page;
uniform int chnl;

uniform vec4 textColor;

in vec2 vfTexCoord;

out vec4 fColor;
 
void main() {
	vec4 texColor = texture(tex, vec3(vfTexCoord, page));
	vec4 glyphColor;
	if (isPacked) {
		float alpha = texColor[chnl];
		if (hasOutline) {
			float val = alpha > 0.5 ? 2 * alpha - 1 : 0;
			glyphColor.rgb = vec3(val);
			glyphColor.a = alpha > 0.5 ? 1 : 2 * alpha;
		} else {
			glyphColor.rgb = vec3(1.0);
			glyphColor.a = alpha;
		}
	} else {
		glyphColor = texColor;
	}
    fColor = glyphColor * textColor;
}