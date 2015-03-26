#version 330 core

uniform sampler2DArray tex;
uniform bool isPacked;
uniform int page;
uniform int chnl;

in vec4 vfColor;
in vec2 vfTexCoord;

out vec4 fColor;
 
void main() {
	vec4 glyphColor;
	vec4 texColor = texture(tex, vec3(vfTexCoord, page));
	if (isPacked) {
		float color = texColor[chnl];
		glyphColor = vec4(vec3(color), 1.0);
	} else {
		glyphColor = texColor;
	}
    fColor = vfColor * glyphColor;
    fColor = vec4(1.0, 0.0, 1.0, 1.0);
}