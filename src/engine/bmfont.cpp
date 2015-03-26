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

#include "bmfont.hpp"
#include "unicode_int.hpp"

#include "logging.hpp"
#include "macros.hpp"

#include <SDL2/SDL_image.h>
#include <GL/glew.h>
#include <fstream>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

// Implement private helper classes for loading the bitmap font files

class BMFontLoader {
public:
    BMFontLoader(FILE *f, BMFont *font, const char *fontFile);

    virtual int Load() = 0; // Must be implemented by derived class

protected:
    void LoadPage(int id, const char *pageFile, const char *fontFile);
    void SetFontInfo(int outlineThickness);
    void SetCommonInfo(int fontHeight, int base, int scaleW, int scaleH, int pages, bool isPacked);
    void AddChar(int id, int x, int y, int w, int h, int xoffset, int yoffset, int xadvance, int page, int chnl);
    void AddKerningPair(int first, int second, int amount);

    FILE *f;
    BMFont *font;
    const char *fontFile;

    int outlineThickness;
};

class BMFontLoaderTextFormat : public BMFontLoader {
public:
    BMFontLoaderTextFormat(FILE *f, BMFont *font, const char *fontFile);

    int Load();

    int SkipWhiteSpace(std::string &str, int start);
    int FindEndOfToken(std::string &str, int start);

    void InterpretInfo(std::string &str, int start);
    void InterpretCommon(std::string &str, int start);
    void InterpretChar(std::string &str, int start);
    void InterpretSpacing(std::string &str, int start);
    void InterpretKerning(std::string &str, int start);
    void InterpretPage(std::string &str, int start, const char *fontFile);
};

class BMFontLoaderBinaryFormat : public BMFontLoader {
public:
    BMFontLoaderBinaryFormat(FILE *f, BMFont *font, const char *fontFile);

    int Load();

    void ReadInfoBlock(int size);
    void ReadCommonBlock(int size);
    void ReadPagesBlock(int size);
    void ReadCharsBlock(int size);
    void ReadKerningPairsBlock(int size);
};

//=============================================================================
// BMFont
//
// This is the BMFont class that is used to write text with bitmap fonts.
//=============================================================================

BMFont::BMFont()
{
    fontHeight = 0;
    base = 0;
    scaleW = 0;
    scaleH = 0;
    scale = 1.0f;
    hasOutline = false;
    encoding = NONE;
}

BMFont::~BMFont()
{
    std::map<int, CharDesc*>::iterator it = chars.begin();
    while (it != chars.end()) {
        delete it->second;
        it++;
    }

    glDeleteTextures(1, &tex);
    glDeleteProgram(program);
}

int BMFont::load(const char *fontFile)
{
    // Load the font
    FILE *f = fopen(fontFile, "rb");

    // Determine format by reading the first bytes of the file
    char str[4] = { 0 };
    fread(str, 3, 1, f);
    fseek(f, 0, SEEK_SET);

    BMFontLoader *loader = 0;
    if (strcmp(str, "BMF") == 0)
        loader = new BMFontLoaderBinaryFormat(f, this, fontFile);
    else
        loader = new BMFontLoaderTextFormat(f, this, fontFile);

    auto r = loader->Load();
    delete loader;

    // build vbo for all glyphs
    glBindVertexArray(0);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW);
    logOpenGLError();

    // build huge fucking array here
    size_t bufferSize = chars.size() * 6 * 4;
    float *vboBuffer = new float[bufferSize];
    float *head = vboBuffer;
    for (auto &descPair : chars) {
        auto desc = descPair.second;

        *head++ = 0.0f;
        *head++ = 0.0f;
        *head++ = (float)desc->srcX;
        *head++ = (float)desc->srcY;

        *head++ = (float)desc->srcW;
        *head++ = 0.0f;
        *head++ = (float)(desc->srcX + desc->srcW);
        *head++ = desc->srcY;

        *head++ = (float)desc->srcW;
        *head++ = (float)desc->srcH;
        *head++ = (float)(desc->srcX + desc->srcW);
        *head++ = (float)(desc->srcY + desc->srcH);

        *head++ = (float)desc->srcW;
        *head++ = (float)desc->srcH;
        *head++ = (float)(desc->srcX + desc->srcW);
        *head++ = (float)(desc->srcY + desc->srcH);

        *head++ = 0.0f;
        *head++ = (float)desc->srcH;
        *head++ = (float)desc->srcX;
        *head++ = (float)(desc->srcY + desc->srcH);

        *head++ = 0.0f;
        *head++ = 0.0f;
        *head++ = (float)desc->srcX;
        *head++ = (float)desc->srcY;

        desc->vboOffset = (head - vboBuffer) * sizeof (float);
    }
    glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(float), vboBuffer, GL_STATIC_DRAW);
    logOpenGLError();

    PrepareEffect();

    return r;
}

void BMFont::SetTextEncoding(FontTextEncoding encoding)
{
    this->encoding = encoding;
}

// Internal
CharDesc *BMFont::GetChar(int id)
{
    std::map<int, CharDesc*>::iterator it = chars.find(id);
    if (it == chars.end())
        return 0;
    return it->second;
}

// Internal
float BMFont::AdjustForKerningPairs(int first, int second)
{
    CharDesc *ch = GetChar(first);
    if (ch == 0) return 0;
    for (uint n = 0; n < ch->kerningPairs.size(); n += 2) {
        if (ch->kerningPairs[n] == second)
            return ch->kerningPairs[n + 1] * scale;
    }

    return 0;
}

float BMFont::GetTextWidth(const char *text, int count)
{
    if (count <= 0)
        count = GetTextLength(text);

    float x = 0;

    for (int n = 0; n < count;) {
        int charId = GetTextChar(text, n, &n);

        CharDesc *ch = GetChar(charId);
        if (ch == 0) ch = &defChar;

        x += scale * (ch->xAdv);

        if (n < count)
            x += AdjustForKerningPairs(charId, GetTextChar(text, n));
    }

    return x;
}

void BMFont::SetHeight(float h)
{
    scale = h / float(fontHeight);
}

float BMFont::GetHeight()
{
    return scale * float(fontHeight);
}

float BMFont::GetBottomOffset()
{
    return scale * (base - fontHeight);
}

float BMFont::GetTopOffset()
{
    return scale * (base - 0);
}

// Internal
// Returns the number of bytes in the string until the null char
int BMFont::GetTextLength(const char *text)
{
    if (encoding == UTF16) {
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

// Internal
int BMFont::GetTextChar(const char *text, int pos, int *nextPos)
{
    int ch;
    unsigned int len;
    if (encoding == UTF8) {
        ch = decodeUTF8(&text[pos], &len);
        if (ch == -1) len = 1;
    } else if (encoding == UTF16) {
        ch = decodeUTF16(&text[pos], &len);
        if (ch == -1) len = 2;
    } else {
        len = 1;
        ch = (unsigned char)text[pos];
    }

    if (nextPos) *nextPos = pos + len;
    return ch;
}

// Internal
int BMFont::FindTextChar(const char *text, int start, int length, int ch)
{
    int pos = start;
    int nextPos;
    int currChar = -1;
    while (pos < length) {
        currChar = GetTextChar(text, pos, &nextPos);
        if (currChar == ch)
            return pos;
        pos = nextPos;
    }

    return -1;
}

void BMFont::InternalWrite(float x, float y, float z, const char *text, int count, float spacing)
{
    int page = -1;

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0); // coord
    glEnableVertexAttribArray(1); // texCoord
    glUseProgram(program);
    logOpenGLError();

    glDisable(GL_DEPTH_TEST);

    y += scale * float(base);

    for (int n = 0; n < count;) {
        int charId = GetTextChar(text, n, &n);
        CharDesc *ch = GetChar(charId);
        if (ch == 0) ch = &defChar;

        // Map the center of the texel to the corners
        // in order to get pixel perfect mapping
        float u = (float(ch->srcX) + 0.5f) / scaleW;
        float v = (float(ch->srcY) + 0.5f) / scaleH;
        float u2 = u + float(ch->srcW) / scaleW;
        float v2 = v + float(ch->srcH) / scaleH;

        float a = scale * float(ch->xAdv);
        float w = scale * float(ch->srcW);
        float h = scale * float(ch->srcH);
        float ox = scale * float(ch->xOff);
        float oy = scale * float(ch->yOff);

        logOpenGLError();

        void *coordOffset = (void *)ch->vboOffset;
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), coordOffset);

        void *texCoordOffset = (void *)(ch->vboOffset + 2 * sizeof(float));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), texCoordOffset);
        logOpenGLError();

        GLuint uniform;
        uniform = glGetUniformLocation(program, "isPacked");
        glUniform1i(uniform, isPacked);
        uniform = glGetUniformLocation(program, "page");
        glUniform1i(uniform, ch->page);
        uniform = glGetUniformLocation(program, "chnl");
        glUniform1i(uniform, ch->chnl);

        glm::mat4 projectionMatrix = glm::mat4(1.0f);
        //glm::translate(projectionMatrix, glm::vec3(x + ox, y + oy, 0.0f));
        uniform = glGetUniformLocation(program, "projectionMatrix");
        glUniformMatrix4fv(uniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        logOpenGLError();

        float buffer[] = {0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0};
        glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), buffer, GL_STATIC_DRAW);
        
        glUseProgram(9);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        logOpenGLError();
        glUseProgram(program);

        x += a;
        if (charId == ' ')
            x += spacing;

        if (n < count)
            x += AdjustForKerningPairs(charId, GetTextChar(text, n));
    }
}

void BMFont::Write(float x, float y, float z, const char *text, int count, unsigned int mode)
{
    if (count <= 0)
        count = GetTextLength(text);

    if (mode == FONT_ALIGN_CENTER) {
        float w = GetTextWidth(text, count);
        x -= w / 2;
    } else if (mode == FONT_ALIGN_RIGHT) {
        float w = GetTextWidth(text, count);
        x -= w;
    }

    InternalWrite(x, y, z, text, count);
}

void BMFont::WriteML(float x, float y, float z, const char *text, int count, unsigned int mode)
{
    if (count <= 0)
        count = GetTextLength(text);

    // Get first line
    int pos = 0;
    int len = FindTextChar(text, pos, count, '\n');
    if (len == -1) len = count;
    while (pos < count) {
        float cx = x;
        if (mode == FONT_ALIGN_CENTER) {
            float w = GetTextWidth(&text[pos], len);
            cx -= w / 2;
        } else if (mode == FONT_ALIGN_RIGHT) {
            float w = GetTextWidth(&text[pos], len);
            cx -= w;
        }

        InternalWrite(cx, y, z, &text[pos], len);

        y -= scale * float(fontHeight);

        // Get next line
        pos += len;
        int ch = GetTextChar(text, pos, &pos);
        if (ch == '\n') {
            len = FindTextChar(text, pos, count, '\n');
            if (len == -1)
                len = count - pos;
            else
                len = len - pos;
        }
    }
}

void BMFont::WriteBox(float x, float y, float z, float width, const char *text, int count, unsigned int mode)
{
    if (count <= 0)
        count = GetTextLength(text);

    float currWidth = 0, wordWidth;
    int lineS = 0, lineE = 0, wordS = 0, wordE = 0;
    int wordCount = 0;

    const char *s = " ";
    float spaceWidth = GetTextWidth(s, 1);
    bool softBreak = false;

    for (; lineS < count;) {
        // Determine the extent of the line
        for (;;) {
            // Determine the number of characters in the word
            while (wordE < count &&
                GetTextChar(text, wordE) != ' ' &&
                GetTextChar(text, wordE) != '\n')
                // Advance the cursor to the next character
                GetTextChar(text, wordE, &wordE);

            // Determine the width of the word
            if (wordE > wordS) {
                wordCount++;
                wordWidth = GetTextWidth(&text[wordS], wordE - wordS);
            } else
                wordWidth = 0;

            // Does the word fit on the line? The first word is always accepted.
            if (wordCount == 1 || currWidth + (wordCount > 1 ? spaceWidth : 0) + wordWidth <= width) {
                // Increase the line extent to the end of the word
                lineE = wordE;
                currWidth += (wordCount > 1 ? spaceWidth : 0) + wordWidth;

                // Did we reach the end of the line?
                if (wordE == count || GetTextChar(text, wordE) == '\n') {
                    softBreak = false;

                    // Skip the newline character
                    if (wordE < count)
                        // Advance the cursor to the next character
                        GetTextChar(text, wordE, &wordE);

                    break;
                }

                // Skip the trailing space
                if (wordE < count && GetTextChar(text, wordE) == ' ')
                    // Advance the cursor to the next character
                    GetTextChar(text, wordE, &wordE);

                // Move to next word
                wordS = wordE;
            } else {
                softBreak = true;

                // Skip the trailing space
                if (wordE < count && GetTextChar(text, wordE) == ' ')
                    // Advance the cursor to the next character
                    GetTextChar(text, wordE, &wordE);

                break;
            }
        }

        // Write the line
        if (mode == FONT_ALIGN_JUSTIFY) {
            float spacing = 0;
            if (softBreak) {
                if (wordCount > 2)
                    spacing = (width - currWidth) / (wordCount - 2);
                else
                    spacing = (width - currWidth);
            }

            InternalWrite(x, y, z, &text[lineS], lineE - lineS, spacing);
        } else {
            float cx = x;
            if (mode == FONT_ALIGN_RIGHT)
                cx = x + width - currWidth;
            else if (mode == FONT_ALIGN_CENTER)
                cx = x + 0.5f*(width - currWidth);

            InternalWrite(cx, y, z, &text[lineS], lineE - lineS);
        }

        if (softBreak) {
            // Skip the trailing space
            if (lineE < count && GetTextChar(text, lineE) == ' ')
                // Advance the cursor to the next character
                GetTextChar(text, lineE, &lineE);

            // We've already counted the first word on the next line
            currWidth = wordWidth;
            wordCount = 1;
        } else {
            // Skip the line break
            if (lineE < count && GetTextChar(text, lineE) == '\n')
                // Advance the cursor to the next character
                GetTextChar(text, lineE, &lineE);

            currWidth = 0;
            wordCount = 0;
        }

        // Move to next line
        lineS = lineE;
        wordS = wordE;
        y -= scale * float(fontHeight);
    }
}

void BMFont::PrepareEffect()
{
    auto compileShaderLambda = [&] (GLuint shader, const char *path) {
        using namespace std;

        ifstream ifs(path, ios::in);
        if (!ifs.is_open()) {
            LOG(ERROR, "Could not open '" << path << "'!");
            return false;
        }
        stringstream ss;
        ss << ifs.rdbuf();
        ifs.close();

        string sourceStr = ss.str();
        char const *source = sourceStr.c_str();
        glShaderSource(shader, 1, &source, NULL);
        glCompileShader(shader);
        GLint shaderStatus = GL_FALSE;
        int infoLogLength;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderStatus);
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        if (shaderStatus) {
            LOG(INFO, "Shader '" << path << "' compiled successfully!");
        } else {
            char *infoLog = new char[infoLogLength];
            glGetShaderInfoLog(shader, infoLogLength, NULL, infoLog);
            LOG(ERROR, "Shader '" << path << "' did not compile!");
            LOG(DEBUG, infoLog);
            delete[] infoLog;
        }

        return !shaderStatus;
    };

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    compileShaderLambda(vert, "shaders/font.vert");

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    compileShaderLambda(frag, "shaders/font.frag");

    program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint programStatus = GL_FALSE;
    int infoLogLength;
    glGetProgramiv(program, GL_LINK_STATUS, &programStatus);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

    if (programStatus) {
        LOG(INFO, "Shader program linked successfully!");
    } else {
        char *infoLog = new char[infoLogLength];
        glGetProgramInfoLog(program, infoLogLength, NULL, infoLog);
        LOG(ERROR, "Shader program did not link!");
        LOG(DEBUG, infoLog);
        delete[] infoLog;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);

    logOpenGLError();
}

void BMFont::PreparePixelPerfectOutput()
{
    // nothing
}

//=============================================================================
// BMFontLoader
//
// This is the base class for all loader classes. This is the only class
// that has access to and knows how to set the BMFont members.
//=============================================================================

BMFontLoader::BMFontLoader(FILE *f, BMFont *font, const char *fontFile)
{
    this->f = f;
    this->font = font;
    this->fontFile = fontFile;

    outlineThickness = 0;
}

void BMFontLoader::LoadPage(int id, const char *pageFile, const char *fontFile)
{
    string str;
    SDL_Surface *img = nullptr, *tmp = nullptr;

    // Load the texture from the same directory as the font descriptor file
    str = fontFile;
    for (size_t n = 0; (n = str.find('/', n)) != string::npos;) str.replace(n, 1, "\\");
    size_t i = str.rfind('\\');
    if (i != string::npos)
        str = str.substr(0, i + 1);
    else
        str = "";
    str += pageFile;

    // Load the font textures
    img = IMG_Load("img/textures_1.png");
    if (!img) {
        LOG(ERROR, "Textures could not be loaded");
        goto FAILURE;
    }
    tmp = SDL_CreateRGBSurface(
        0, font->scaleW, font->scaleH, 32,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
    if (!tmp) {
        LOG(ERROR, "Temporary SDL_Surface could not be created");
        goto FAILURE;
    }
    {
        SDL_Rect rect{ 0, 0, font->scaleW, font->scaleH };
        int ret_code = SDL_BlitSurface(img, &rect, tmp, nullptr);
        if (ret_code) {
            LOG(ERROR, "Blit unsuccessful: " << SDL_GetError());
            goto FAILURE;
        }
    }
    SDL_FreeSurface(img);
    glBindTexture(GL_TEXTURE_2D_ARRAY, font->tex);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, id, font->scaleW, font->scaleH, 1, GL_RGBA, GL_UNSIGNED_BYTE, tmp->pixels);
    logOpenGLError();
    SDL_FreeSurface(tmp);
    return;

FAILURE:
    LOG(ERROR, "Failed to load font page '" << str.c_str() << "'");
    if (img) SDL_FreeSurface(img);
    if (tmp) SDL_FreeSurface(tmp);
    return;
}

void BMFontLoader::SetFontInfo(int outlineThickness)
{
    this->outlineThickness = outlineThickness;
}

void BMFontLoader::SetCommonInfo(int fontHeight, int base, int scaleW, int scaleH, int pages, bool isPacked)
{
    font->fontHeight = fontHeight;
    font->base = base;
    font->scaleW = scaleW;
    font->scaleH = scaleH;
    font->pages = pages;
    font->isPacked = isPacked;
    if (isPacked && outlineThickness)
        font->hasOutline = true;

    logOpenGLError();

    glGenTextures(1, &font->tex);
    logOpenGLError();
    glBindTexture(GL_TEXTURE_2D_ARRAY, font->tex);
    logOpenGLError();
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, scaleW, scaleH, pages);
    logOpenGLError();

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    logOpenGLError();
}

void BMFontLoader::AddChar(int id, int x, int y, int w, int h, int xoffset, int yoffset, int xadvance, int page, int chnl)
{
    // Convert to a 4 element vector
    // TODO: Does this depend on hardware? It probably does
    if (chnl == 1) chnl = 0x00010000;  // Blue channel
    else if (chnl == 2) chnl = 0x00000100;  // Green channel
    else if (chnl == 4) chnl = 0x00000001;  // Red channel
    else if (chnl == 8) chnl = 0x01000000;  // Alpha channel
    else chnl = 0;

    if (id >= 0) {
        CharDesc *ch = new CharDesc{
            x, y, w, h, xoffset, yoffset, xadvance, page, chnl
        };
        font->chars.insert(std::map<int, CharDesc*>::value_type(id, ch));
    }

    if (id == -1) {
        font->defChar = CharDesc{
            x, y, w, h, xoffset, yoffset, xadvance, page, chnl
        };
    }
}

void BMFontLoader::AddKerningPair(int first, int second, int amount)
{
    if (first >= 0 && first < 256 && font->chars[first]) {
        font->chars[first]->kerningPairs.push_back(second);
        font->chars[first]->kerningPairs.push_back(amount);
    }
}

//=============================================================================
// CFontLoaderTextFormat
//
// This class implements the logic for loading a BMFont file in text format
//=============================================================================

BMFontLoaderTextFormat::BMFontLoaderTextFormat(FILE *f, BMFont *font, const char *fontFile) : BMFontLoader(f, font, fontFile)
{
    // nothing
}

int BMFontLoaderTextFormat::Load()
{
    string line;

    while (!feof(f)) {
        // Read until line feed (or EOF)
        line = "";
        line.reserve(256);
        while (!feof(f)) {
            char ch;
            if (fread(&ch, 1, 1, f)) {
                if (ch != '\n')
                    line += ch;
                else
                    break;
            }
        }

        // Skip white spaces
        int pos = SkipWhiteSpace(line, 0);

        // Read token
        int pos2 = FindEndOfToken(line, pos);
        string token = line.substr(pos, pos2 - pos);

        // Interpret line
        if (token == "info")
            InterpretInfo(line, pos2);
        else if (token == "common")
            InterpretCommon(line, pos2);
        else if (token == "char")
            InterpretChar(line, pos2);
        else if (token == "kerning")
            InterpretKerning(line, pos2);
        else if (token == "page")
            InterpretPage(line, pos2, fontFile);
    }

    fclose(f);

    // Success
    return 0;
}

int BMFontLoaderTextFormat::SkipWhiteSpace(string &str, int start)
{
    uint n = start;
    while (n < str.size()) {
        char ch = str[n];
        if (ch != ' ' &&
            ch != '\t' &&
            ch != '\r' &&
            ch != '\n')
            break;

        ++n;
    }

    return n;
}

int BMFontLoaderTextFormat::FindEndOfToken(string &str, int start)
{
    uint n = start;
    if (str[n] == '"') {
        n++;
        while (n < str.size()) {
            char ch = str[n];
            if (ch == '"') {
                // Include the last quote char in the token
                ++n;
                break;
            }
            ++n;
        }
    } else {
        while (n < str.size()) {
            char ch = str[n];
            if (ch == ' ' ||
                ch == '\t' ||
                ch == '\r' ||
                ch == '\n' ||
                ch == '=')
                break;

            ++n;
        }
    }

    return n;
}

void BMFontLoaderTextFormat::InterpretKerning(string &str, int start)
{
    // Read the attributes
    int first = 0;
    int second = 0;
    int amount = 0;

    int pos, pos2 = start;
    while (true) {
        pos = SkipWhiteSpace(str, pos2);
        pos2 = FindEndOfToken(str, pos);

        string token = str.substr(pos, pos2 - pos);

        pos = SkipWhiteSpace(str, pos2);
        if (pos == str.size() || str[pos] != '=') break;

        pos = SkipWhiteSpace(str, pos + 1);
        pos2 = FindEndOfToken(str, pos);

        string value = str.substr(pos, pos2 - pos);

        if (token == "first")
            first = strtol(value.c_str(), 0, 10);
        else if (token == "second")
            second = strtol(value.c_str(), 0, 10);
        else if (token == "amount")
            amount = strtol(value.c_str(), 0, 10);

        if (pos == str.size()) break;
    }

    // Store the attributes
    AddKerningPair(first, second, amount);
}

void BMFontLoaderTextFormat::InterpretChar(string &str, int start)
{
    // Read all attributes
    int id = 0;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    int xoffset = 0;
    int yoffset = 0;
    int xadvance = 0;
    int page = 0;
    int chnl = 0;

    int pos, pos2 = start;
    while (true) {
        pos = SkipWhiteSpace(str, pos2);
        pos2 = FindEndOfToken(str, pos);

        string token = str.substr(pos, pos2 - pos);

        pos = SkipWhiteSpace(str, pos2);
        if (pos == str.size() || str[pos] != '=') break;

        pos = SkipWhiteSpace(str, pos + 1);
        pos2 = FindEndOfToken(str, pos);

        string value = str.substr(pos, pos2 - pos);

        if (token == "id")
            id = strtol(value.c_str(), 0, 10);
        else if (token == "x")
            x = strtol(value.c_str(), 0, 10);
        else if (token == "y")
            y = strtol(value.c_str(), 0, 10);
        else if (token == "width")
            width = strtol(value.c_str(), 0, 10);
        else if (token == "height")
            height = strtol(value.c_str(), 0, 10);
        else if (token == "xoffset")
            xoffset = strtol(value.c_str(), 0, 10);
        else if (token == "yoffset")
            yoffset = strtol(value.c_str(), 0, 10);
        else if (token == "xadvance")
            xadvance = strtol(value.c_str(), 0, 10);
        else if (token == "page")
            page = strtol(value.c_str(), 0, 10);
        else if (token == "chnl")
            chnl = strtol(value.c_str(), 0, 10);

        if (pos == str.size()) break;
    }

    // Store the attributes
    AddChar(id, x, y, width, height, xoffset, yoffset, xadvance, page, chnl);
}

void BMFontLoaderTextFormat::InterpretCommon(string &str, int start)
{
    int fontHeight;
    int base;
    int scaleW;
    int scaleH;
    int pages;
    int packed;

    // Read all attributes
    int pos, pos2 = start;
    while (true) {
        pos = SkipWhiteSpace(str, pos2);
        pos2 = FindEndOfToken(str, pos);

        string token = str.substr(pos, pos2 - pos);

        pos = SkipWhiteSpace(str, pos2);
        if (pos == str.size() || str[pos] != '=') break;

        pos = SkipWhiteSpace(str, pos + 1);
        pos2 = FindEndOfToken(str, pos);

        string value = str.substr(pos, pos2 - pos);

        if (token == "lineHeight")
            fontHeight = (short)strtol(value.c_str(), 0, 10);
        else if (token == "base")
            base = (short)strtol(value.c_str(), 0, 10);
        else if (token == "scaleW")
            scaleW = (short)strtol(value.c_str(), 0, 10);
        else if (token == "scaleH")
            scaleH = (short)strtol(value.c_str(), 0, 10);
        else if (token == "pages")
            pages = strtol(value.c_str(), 0, 10);
        else if (token == "packed")
            packed = strtol(value.c_str(), 0, 10);

        if (pos == str.size()) break;
    }

    SetCommonInfo(fontHeight, base, scaleW, scaleH, pages, packed ? true : false);
}

void BMFontLoaderTextFormat::InterpretInfo(string &str, int start)
{
    int outlineThickness;

    // Read all attributes
    int pos, pos2 = start;
    while (true) {
        pos = SkipWhiteSpace(str, pos2);
        pos2 = FindEndOfToken(str, pos);

        string token = str.substr(pos, pos2 - pos);

        pos = SkipWhiteSpace(str, pos2);
        if (pos == str.size() || str[pos] != '=') break;

        pos = SkipWhiteSpace(str, pos + 1);
        pos2 = FindEndOfToken(str, pos);

        string value = str.substr(pos, pos2 - pos);

        if (token == "outline")
            outlineThickness = (short)strtol(value.c_str(), 0, 10);

        if (pos == str.size()) break;
    }

    SetFontInfo(outlineThickness);
}

void BMFontLoaderTextFormat::InterpretPage(string &str, int start, const char *fontFile)
{
    int id = 0;
    string file;

    // Read all attributes
    int pos, pos2 = start;
    while (true) {
        pos = SkipWhiteSpace(str, pos2);
        pos2 = FindEndOfToken(str, pos);

        string token = str.substr(pos, pos2 - pos);

        pos = SkipWhiteSpace(str, pos2);
        if (pos == str.size() || str[pos] != '=') break;

        pos = SkipWhiteSpace(str, pos + 1);
        pos2 = FindEndOfToken(str, pos);

        string value = str.substr(pos, pos2 - pos);

        if (token == "id")
            id = strtol(value.c_str(), 0, 10);
        else if (token == "file")
            file = value.substr(1, value.length() - 2);

        if (pos == str.size()) break;
    }

    LoadPage(id, file.c_str(), fontFile);
}

//=============================================================================
// BMFontLoaderBinaryFormat
//
// This class implements the logic for loading a BMFont file in binary format
//=============================================================================

BMFontLoaderBinaryFormat::BMFontLoaderBinaryFormat(FILE *f, BMFont *font, const char *fontFile) : BMFontLoader(f, font, fontFile)
{
    // nothing
}

int BMFontLoaderBinaryFormat::Load()
{
    // Read and validate the tag. It should be 66, 77, 70, 2, 
    // or 'BMF' and 2 where the number is the file version.
    char magicString[4];
    fread(magicString, 4, 1, f);
    if (strncmp(magicString, "BMF\003", 4) != 0) {
        //LOG(("Unrecognized format for '%s'", fontFile));
        fclose(f);
        return -1;
    }

    // Read each block
    char blockType;
    int blockSize;
    while (fread(&blockType, 1, 1, f)) {
        // Read the blockSize
        fread(&blockSize, 4, 1, f);

        switch (blockType) {
        case 1: // info
            ReadInfoBlock(blockSize);
            break;
        case 2: // common
            ReadCommonBlock(blockSize);
            break;
        case 3: // pages
            ReadPagesBlock(blockSize);
            break;
        case 4: // chars
            ReadCharsBlock(blockSize);
            break;
        case 5: // kerning pairs
            ReadKerningPairsBlock(blockSize);
            break;
        default:
            //LOG(("Unexpected block type (%d)", blockType));
            fclose(f);
            return -1;
        }
    }

    fclose(f);

    // Success
    return 0;
}

void BMFontLoaderBinaryFormat::ReadInfoBlock(int size)
{
    PACKED(
    struct infoBlock {
        uint16 fontSize;
        uint   reserved : 4;
        uint   bold : 1;
        uint   italic : 1;
        uint   unicode : 1;
        uint   smooth : 1;
        uint8  charSet;
        uint16 stretchH;
        uint8  aa;
        uint8  paddingUp;
        uint8  paddingRight;
        uint8  paddingDown;
        uint8  paddingLeft;
        uint8  spacingHoriz;
        uint8  spacingVert;
        uint8  outline; // Added with version 2
        char fontName[1];
    });

    char *buffer = new char[size];
    fread(buffer, size, 1, f);

    // We're only interested in the outline thickness
    infoBlock *blk = (infoBlock*)buffer;
    SetFontInfo(blk->outline);

    delete[] buffer;
}

void BMFontLoaderBinaryFormat::ReadCommonBlock(int size)
{

    PACKED(
    struct commonBlock {
        uint16 lineHeight;
        uint16 base;
        uint16 scaleW;
        uint16 scaleH;
        uint16 pages;
        uint8 packed : 1;
        uint8 reserved : 7;
        uint8 alphaChnl;
        uint8 redChnl;
        uint8 greenChnl;
        uint8 blueChnl;
    });

    char *buffer = new char[size];
    fread(buffer, size, 1, f);

    commonBlock *blk = (commonBlock*)buffer;

    SetCommonInfo(blk->lineHeight, blk->base, blk->scaleW, blk->scaleH, blk->pages, blk->packed ? true : false);

    delete[] buffer;
}

void BMFontLoaderBinaryFormat::ReadPagesBlock(int size)
{

    PACKED(
    struct pagesBlock {
        char pageNames[1];
    });

    char *buffer = new char[size];
    fread(buffer, size, 1, f);

    pagesBlock *blk = (pagesBlock*)buffer;

    for (int id = 0, pos = 0; pos < size; id++) {
        LoadPage(id, &blk->pageNames[pos], fontFile);
        pos += 1 + (int)strlen(&blk->pageNames[pos]);
    }

    delete[] buffer;
}

void BMFontLoaderBinaryFormat::ReadCharsBlock(int size)
{

    PACKED(
    struct charsBlock {
        struct charInfo {
            uint32 id;
            uint16 x;
            uint16 y;
            uint16 width;
            uint16 height;
            int16  xoffset;
            int16  yoffset;
            int16  xadvance;
            uint8  page;
            uint8  chnl;
        } chars[1];
    });

    char *buffer = new char[size];
    fread(buffer, size, 1, f);

    charsBlock *blk = (charsBlock*)buffer;

    for (int n = 0; int(n*sizeof(charsBlock::charInfo)) < size; n++) {
        AddChar(blk->chars[n].id,
            blk->chars[n].x,
            blk->chars[n].y,
            blk->chars[n].width,
            blk->chars[n].height,
            blk->chars[n].xoffset,
            blk->chars[n].yoffset,
            blk->chars[n].xadvance,
            blk->chars[n].page,
            blk->chars[n].chnl);
    }

    delete[] buffer;
}

void BMFontLoaderBinaryFormat::ReadKerningPairsBlock(int size)
{
    PACKED(
    struct kerningPairsBlock {
        struct kerningPair {
            uint32 first;
            uint32 second;
            int16  amount;
        } kerningPairs[1];
    });

    char *buffer = new char[size];
    fread(buffer, size, 1, f);

    kerningPairsBlock *blk = (kerningPairsBlock*)buffer;

    for (int n = 0; int(n*sizeof(kerningPairsBlock::kerningPair)) < size; n++) {
        AddKerningPair(blk->kerningPairs[n].first,
            blk->kerningPairs[n].second,
            blk->kerningPairs[n].amount);
    }

    delete[] buffer;
}

// 2008-05-11 Storing the characters in a map instead of an array
// 2008-05-11 Loading the new binary format for BMFont v1.10
// 2008-05-17 Added support for writing text with UTF8 and UTF16 encoding
