#version 330 core

uniform sampler2DArray tex;
uniform bool isPacked;
uniform bool hasOutline;
uniform int page;
uniform int chnl;

uniform vec4 textColor;
uniform vec4 outlineColor;
uniform int mode;

in vec2 vfTexCoord;

out vec4 fColor;

vec4 computeWithOutline(float val) {
	bool isInsideGlyph = val > 0.5;
	float val1 = isInsideGlyph ? 2 * val - 1 : 0;
	float val2 = isInsideGlyph ? 1 : 2 * val;
	switch (mode) {
		default:
			// render both
			vec4 color = val1 * textColor + (1 - val1) * outlineColor;
			return vec4(color.rgb, color.a * val2);

		case 1:
			// render only outline
			return vec4(outlineColor.rgb, outlineColor.a * val2);

		case 2:
			// render only text
			return vec4(textColor.rgb, textColor.a * val1);
	}
}

vec4 computePacked(vec4 texColor) {
	float val = texColor[chnl];
	if (hasOutline) {
		return computeWithOutline(val);
	} else {
		return vec4(vec3(1.0), val);
	}
}

void main() {
	vec4 texColor = texture(tex, vec3(vfTexCoord, page));
	if (isPacked) {
		fColor = computePacked(texColor);
	} else {
		fColor = texColor * textColor;
	}
}