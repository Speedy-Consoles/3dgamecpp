/*
    AngelCode Tool Box Library
    Copyright (c) 2007-2008 Andreas Jonsson
  
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
  
    Andreas Jonsson
    andreas@angelcode.com
*/

/*
    This file was modified from its original version!
    The original version could be obtained from
        http://www.angelcode.com/dev/bmfonts/
    as of 2015-03-22

    2015-03-22 - Lars Thorben Drögemüller
        Removed enclosing namespace and enforced coding conventions
        All the DirectX specific code was thrown out
*/

#ifndef BMFONT_HPP
#define BMFONT_HPP

#include <vector>
#include <string>
#include <map>

#include <GL/glew.h>

#include "std_types.hpp"

struct CharDesc {
    short srcX;
    short srcY;
    short srcW;
    short srcH;
    short xOff;
    short yOff;
    short xAdv;
    short page;
    unsigned int chnl;

    unsigned int vboOffset;

    std::vector<int> kerningPairs;
};

enum FontTextEncoding
{
	NONE,
	UTF8,
	UTF16
};

class BMFontLoader;

class BMFont
{
public:
    BMFont();
    ~BMFont();

	int load(const char *fontFile);

    void SetTextEncoding(FontTextEncoding encoding);

	float GetTextWidth(const char *text, int count);
	void Write(float x, float y, float z, const char *text, int count, unsigned int mode);
	void WriteML(float x, float y, float z, const char *text, int count, unsigned int mode);
	void WriteBox(float x, float y, float z, float width, const char *text, int count, unsigned mode);

	void SetHeight(float h);
	float GetHeight();

	float GetBottomOffset();
	float GetTopOffset();

	void PrepareEffect();
	void PreparePixelPerfectOutput();

protected:
    friend class BMFontLoader;

	void InternalWrite(float x, float y, float z, const char *text, int count, float spacing = 0);

	float AdjustForKerningPairs(int first, int second);
    CharDesc *GetChar(int id);

	int GetTextLength(const char *text);
	int GetTextChar(const char *text, int pos, int *nextPos = 0);
	int FindTextChar(const char *text, int start, int length, int ch);

	short fontHeight; // total height of the font
	short base;       // y of base line
	short scaleW;
	short scaleH;
    CharDesc defChar;
    bool isPacked;
	bool hasOutline;

	float scale;
    FontTextEncoding encoding;

    std::map<int, CharDesc*> chars;
    short pages;
    GLuint tex;
    GLuint program;
    GLuint vbo;
};

enum {
    FONT_ALIGN_LEFT,
    FONT_ALIGN_CENTER,
    FONT_ALIGN_RIGHT,
    FONT_ALIGN_JUSTIFY,
};

// 2008-05-11 Storing the characters in a map instead of an array
// 2008-05-17 Added support for writing text with UTF8 and UTF16 encoding

#endif // BMFONT_HPP
