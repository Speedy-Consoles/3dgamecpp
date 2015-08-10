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
	The original filename was acgfx_font.cpp

	2015-03-22 - Lars Thorben Drögemüller
		Removed enclosing namespace and enforced coding conventions
		All the DirectX specific code was thrown out
		Rewrote rendering code in OpenGL 3.3
		Seperated file format logic from rendering logic by subclassing
*/

#include "font.hpp"

#include <cstring>

#include "shared/engine/unicode_int.hpp"

void Font::writeLine(float x, float y, float z, const char *text, int count, Alignment alignment) {
	if (count <= 0)
		count = getTextLength(text);

	if (alignment == Alignment::CENTER) {
		float w = getTextWidth(text, count);
		x -= w / 2;
	} else if (alignment == Alignment::RIGHT) {
		float w = getTextWidth(text, count);
		x -= w;
	}

	beginRender();
	writeInternal(x, y, z, text, count);
	endRender();
}

void Font::write(float x, float y, float z, const char *text, int count, Alignment alignment) {
	if (count <= 0)
		count = getTextLength(text);

	// Get first line
	int pos = 0;
	int len = findTextChar(text, pos, count, '\n');
	if (len == -1) len = count;
	beginRender();
	while (pos < count) {
		float cx = x;
		if (alignment == Alignment::CENTER) {
			float w = getTextWidth(&text[pos], len);
			cx -= w / 2;
		} else if (alignment == Alignment::RIGHT) {
			float w = getTextWidth(&text[pos], len);
			cx -= w;
		}

		writeInternal(cx, y, z, &text[pos], len);

		y -= getLineHeight();

		// Get next line
		pos += len;
		int ch = getTextChar(text, pos, &pos);
		if (ch == '\n') {
			len = findTextChar(text, pos, count, '\n');
			if (len == -1)
				len = count - pos;
			else
				len = len - pos;
		}
	}
	endRender();
}

void Font::writeBox(float x, float y, float z, float width, const char *text, int count, Alignment alignment) {
	if (count <= 0)
		count = getTextLength(text);

	float currWidth = 0, wordWidth;
	int lineS = 0, lineE = 0, wordS = 0, wordE = 0;
	int wordCount = 0;

	const char *s = " ";
	float spaceWidth = getTextWidth(s, 1);
	bool softBreak = false;

	beginRender();
	for (; lineS < count;) {
		// Determine the extent of the line
		for (;;) {
			// Determine the number of characters in the word
			while (wordE < count &&
				getTextChar(text, wordE) != ' ' &&
				getTextChar(text, wordE) != '\n')
				// Advance the cursor to the next character
				getTextChar(text, wordE, &wordE);

			// Determine the width of the word
			if (wordE > wordS) {
				wordCount++;
				wordWidth = getTextWidth(&text[wordS], wordE - wordS);
			} else
				wordWidth = 0;

			// Does the word fit on the line? The first word is always accepted.
			if (wordCount == 1 || currWidth + (wordCount > 1 ? spaceWidth : 0) + wordWidth <= width) {
				// Increase the line extent to the end of the word
				lineE = wordE;
				currWidth += (wordCount > 1 ? spaceWidth : 0) + wordWidth;

				// Did we reach the end of the line?
				if (wordE == count || getTextChar(text, wordE) == '\n') {
					softBreak = false;

					// Skip the newline character
					if (wordE < count)
						// Advance the cursor to the next character
						getTextChar(text, wordE, &wordE);

					break;
				}

				// Skip the trailing space
				if (wordE < count && getTextChar(text, wordE) == ' ')
					// Advance the cursor to the next character
					getTextChar(text, wordE, &wordE);

				// Move to next word
				wordS = wordE;
			} else {
				softBreak = true;

				// Skip the trailing space
				if (wordE < count && getTextChar(text, wordE) == ' ')
					// Advance the cursor to the next character
					getTextChar(text, wordE, &wordE);

				break;
			}
		}

		// Write the line
		if (alignment == Alignment::JUSTIFY) {
			float spacing = 0;
			if (softBreak) {
				if (wordCount > 2)
					spacing = (width - currWidth) / (wordCount - 2);
				else
					spacing = (width - currWidth);
			}

			writeInternal(x, y, z, &text[lineS], lineE - lineS, spacing);
		} else {
			float cx = x;
			if (alignment == Alignment::RIGHT)
				cx = x + width - currWidth;
			else if (alignment == Alignment::CENTER)
				cx = x + 0.5f*(width - currWidth);

			writeInternal(cx, y, z, &text[lineS], lineE - lineS);
		}

		if (softBreak) {
			// Skip the trailing space
			if (lineE < count && getTextChar(text, lineE) == ' ')
				// Advance the cursor to the next character
				getTextChar(text, lineE, &lineE);

			// We've already counted the first word on the next line
			currWidth = wordWidth;
			wordCount = 1;
		} else {
			// Skip the line break
			if (lineE < count && getTextChar(text, lineE) == '\n')
				// Advance the cursor to the next character
				getTextChar(text, lineE, &lineE);

			currWidth = 0;
			wordCount = 0;
		}

		// Move to next line
		lineS = lineE;
		wordS = wordE;
		y -= getLineHeight();
	}
	endRender();
}

// Returns the number of bytes in the string until the null char
int Font::getTextLength(const char *text) {
    if (encoding == Encoding::UTF16) {
        int textLen = 0;
        for (;;) {
            unsigned int len;
            int r = decodeUTF16(&text[textLen], &len);
            if (r > 0)
                textLen += len;
            else if (r < 0)
                textLen++;
            else
                return textLen;
        }
    }

    // Both UTF8 and standard ASCII strings can use strlen
    return (int)strlen(text);
}

int Font::getTextChar(const char *text, int pos, int *nextPos) {
    int ch;
    unsigned int len;
    if (encoding == Encoding::UTF8) {
        ch = decodeUTF8(&text[pos], &len);
        if (ch == -1) len = 1;
    } else if (encoding == Encoding::UTF16) {
        ch = decodeUTF16(&text[pos], &len);
        if (ch == -1) len = 2;
    } else {
        len = 1;
        ch = (unsigned char)text[pos];
    }

    if (nextPos) *nextPos = pos + len;
    return ch;
}

int Font::findTextChar(const char *text, int start, int length, int ch) {
    int pos = start;
    int nextPos;
    int currChar = -1;
    while (pos < length) {
        currChar = getTextChar(text, pos, &nextPos);
        if (currChar == ch)
            return pos;
        pos = nextPos;
    }

    return -1;
}

void Font::writeInternal(float x, float y, float z, const char *text, int count, float spacing) {
	for (int n = 0; n < count;) {
		int glyph = getTextChar(text, n, &n);
		x += renderGlyph(x, y, z, glyph);
		if (glyph == ' ')
			x += spacing;
		if (n < count)
			x += getKerning(glyph, getTextChar(text, n));
	}
}
