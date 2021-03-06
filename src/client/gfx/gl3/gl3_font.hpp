/*
	AngelCode Tool Box Library
	Copyright (c) 2007-2008 Andreas Jönsson
  
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any
	damages arising from the use of this software.

	Permission is granted to anyone to use this software for any
	purpose, including commercial applications, and to alter it and
	redistribute it freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you
	   must not claim that you wrote the original software. If you use
	   this software in a product, an acknowledgment in the product
	   documentation would be appreciated but is not required.

	2. Altered source versions must be plainly marked as such, and
	   must not be misrepresented as being the original software.

	3. This notice may not be removed or altered from any source
	   distribution.
  
	Andreas Jönsson
	andreas@angelcode.com
*/

/*
	This file was modified from its original version!
	The original version could be obtained from
		http://www.angelcode.com/dev/bmfonts/
	as of 2015-03-22
	The original filename was acgfx_font.h

	2015-03-22 - Lars Thorben Drögemüller
		Removed enclosing namespace and enforced coding conventions
		All the DirectX specific code was thrown out
		Rewrote rendering code in OpenGL 3.3
		Seperated file format logic from rendering logic by subclassing
*/

#ifndef BMFONT_HPP
#define BMFONT_HPP

#include "client/gfx/font.hpp"

#include <vector>
#include <string>
#include <map>

#include <GL/glew.h>

#include "shared/engine/std_types.hpp"

#include "gl3_shaders.hpp"

class BMFontLoader;

class BMFont : public Font {
public:

	struct CharDesc {
		short srcX, srcY, srcW, srcH, xOff, yOff, xAdv;
		short page;
		unsigned int chnl;
		unsigned int vboOffset;
		std::vector<int> kerningPairs;
	};

	BMFont(FontShader *shader);
	~BMFont();

	int load(const char *fontFile);

	void setHeight(float h);

	float getBottomOffset();
	float getTopOffset();

	float getLineHeight() override { return scale * (float)lineHeight; }
	float getKerning(int first, int second) override;

	float getTextWidth(const char *text, int count) override;

	void setColor(const glm::vec4 &color) { textColor = color; }
	void setOutlineColor(const glm::vec4 &color) { outlineColor = color; }
	void setOutline(bool val = true) { outline = val; }


private:
	friend class BMFontLoader;

	void beginRender() override;
	float renderGlyph(float x, float y, float z, int glyph) override;
	CharDesc *getChar(int id);

	void writeInternal(float x, float y, float z, const char *text, int count, float spacing = 0.0f) override;

	short lineHeight = 0; // total height of the font
	short base = 0; // y of base line
	short scaleW = 0;
	short scaleH = 0;
	float scale = 1.0f;
	bool isPacked = false;
	bool hasOutline = false;
	short pages = 0;

	FontShader *shader = nullptr;

	glm::vec4 textColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	glm::vec4 outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	bool outline;

	GLuint tex = 0;
	GLuint program = 0;
	GLuint vbo = 0;

	CharDesc defChar;
	std::map<int, CharDesc*> chars;
};

// 2008-05-11 Storing the characters in a map instead of an array
// 2008-05-17 Added support for writing text with UTF8 and UTF16 encoding

#endif // BMFONT_HPP
