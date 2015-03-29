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
	vec4 glyphColor;
	vec4 texColor = texture(tex, vec3(vfTexCoord, page));
	if (isPacked) {
		float alpha = texColor[chnl];
		glyphColor = vec4(vec3(1.0), alpha);
	} else {
		glyphColor = texColor;
	}
    fColor = glyphColor * textColor;
}