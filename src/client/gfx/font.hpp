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

#ifndef FONT_HPP_
#define FONT_HPP_

// Base class for all fonts
// Deals with encoding, kerning and alignment
class Font {
public:

	enum class Encoding { NONE, UTF8, UTF16 };
	enum class Alignment { LEFT, CENTER, RIGHT, JUSTIFY };

	void setEncoding(Encoding encoding) { this->encoding = encoding; }

	void writeLine(float x, float y, float z, const char *text, int count, Alignment alignment = Alignment::LEFT);
	void write(float x, float y, float z, const char *text, int count, Alignment alignment = Alignment::LEFT);
	void writeBox(float x, float y, float z, float width, const char *text, int count, Alignment alignment = Alignment::LEFT);

protected:
	Encoding encoding = Encoding::NONE;

	virtual float getLineHeight() = 0;
	virtual void beginRender() {}
	virtual void endRender() {}
	virtual float renderGlyph(float x, float y, float z, int glyph) = 0;
	virtual float getTextWidth(const char *text, int count) = 0;
	virtual float getKerning(int first, int second) { return 0.0f; }

	int getTextLength(const char *text);
	int getTextChar(const char *text, int pos, int *nextPos = nullptr);
	int findTextChar(const char *text, int start, int length, int ch);

	virtual void writeInternal(float x, float y, float z, const char *text, int count, float spacing = 0.0f);
};

#endif // FONT_HPP_
