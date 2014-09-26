/*
 * texture_manager.cpp
 *
 *  Created on: 19.09.2014
 *      Author: lars
 */

#include "texture_manager.hpp"

#include <SDL2/SDL_image.h>

#include "io/logging.hpp"

static const auto TEX2D = GL_TEXTURE_2D;

TextureManager::TextureManager(const GraphicsConf &conf) :
	conf(conf)
{
	// nothing
}

TextureManager::~TextureManager() {
	for (auto iter : loadedTextures) {
		GLuint tex = iter;
		glDeleteTextures(1, &tex);
	}
}

static void setLoadingOptions(GraphicsConf &conf) {
	bool mipmapping = conf.tex_mipmapping > 0;
	GLenum mag_filter, min_filter;

	switch (conf.tex_filtering) {
	case TexFiltering::LINEAR:
		mag_filter = GL_LINEAR;
		min_filter = mipmapping ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
		break;
	case TexFiltering::NEAREST:
		mag_filter = GL_NEAREST;
		min_filter = mipmapping ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST;
		break;
	default:
		LOG(ERROR, "Reached unreachable code");
		mag_filter = 0;
		min_filter = 0;
		break;
	}
	glTexParameteri(TEX2D, GL_TEXTURE_MAG_FILTER, mag_filter);
	glTexParameteri(TEX2D, GL_TEXTURE_MIN_FILTER, min_filter);
	if (mipmapping) {
		glTexParameteri(TEX2D, GL_TEXTURE_MAX_LEVEL, conf.tex_mipmapping);
		glTexParameteri(TEX2D, GL_GENERATE_MIPMAP, GL_TRUE);
	}
}

void TextureManager::loadTextures(uint *blocks, const char *filename, int xTiles, int yTiles) {
	SDL_Surface *img = IMG_Load(filename);
	if (!img) {
		LOG(ERROR, "File '" << filename << "' could not be loaded");
		return;
	}
	int tileW = img->w / xTiles;
	int tileH = img->h / yTiles;
	SDL_Surface *tmp = SDL_CreateRGBSurface(
			0, tileW, tileH, 32,
			0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	if (!tmp) {
		LOG(ERROR, "Temporary SDL_Surface could not be created");
		return;
	}

	size_t loaded = 0;
	auto blocks_ptr = blocks;
	for (int j = 0; j < yTiles; ++j) {
		for (int i = 0; i < xTiles; ++i) {
			uint block = *blocks_ptr++;
			if (block == 0)
				continue;
			SDL_Rect rect{i * tileW, j * tileH, tileW, tileH};
			int ret_code = SDL_BlitSurface(img, &rect, tmp, nullptr);
			if (ret_code)
				LOG(ERROR, "Blit unsuccessful: " << SDL_GetError());
			loadTexture(block, tmp, SINGLE_TEXTURE);
			++loaded;
		}
	}

	SDL_FreeSurface(tmp);
	SDL_FreeSurface(img);

	GLenum e = glGetError();
	if (e != GL_NO_ERROR) {
		LOG(ERROR, "Loading '" << filename << "': " << gluErrorString(e));
	}

	LOG(DEBUG, "Loaded " << loaded << " textures from '" << filename << "'");
}

GLuint TextureManager::loadTexture(uint block, const char *filename, TextureType type) {
	SDL_Surface *img = IMG_Load(filename);
	if (!img) {
		LOG(ERROR, "File '" << filename << "' could not be opened");
		return 0;
	}
	GLuint tex = loadTexture(block, img, type);
	SDL_FreeSurface(img);
	if (!tex) {
		LOG(ERROR, "File '" << filename << "' could not be opened");
		return 0;
	}
	return tex;
}

GLuint TextureManager::loadTexture(uint block, SDL_Surface *img, TextureType type) {
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(TEX2D, tex);
	setLoadingOptions(conf);
	glTexImage2D(TEX2D, 0, 4, img->w, img->h, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, img->pixels);

	GLenum e = glGetError();
	if (e != GL_NO_ERROR) {
		LOG(ERROR, "Could not load texture: " << gluErrorString(e));
		return 0;
	}

	Entry entry{block, tex, type, 0, 1, 1, -1};
	textures.insert({block, entry});
	loadedTextures.push_back(tex);

	return tex;
}

void TextureManager::setConfig(const GraphicsConf &c) {
	GraphicsConf old_conf = conf;
	conf = c;

	if (conf.tex_mipmapping != old_conf.tex_mipmapping ||
			conf.tex_filtering != old_conf.tex_filtering)
	{
		for (auto iter : loadedTextures) {
			GLuint tex = iter;
			glBindTexture(TEX2D, tex);
			setLoadingOptions(conf);
		}
	}
}

bool TextureManager::bind(uint block) {
	if (block == lastBound.block)
		return false;

	GLuint oldTex = lastBound.tex;

	if (block == 0) {
		lastBound = {0, 0, SINGLE_TEXTURE, 0, 0, 1, 1};
		return oldTex != 0;
	}

	auto iter = textures.find(block);
	if (iter == textures.end()) {
		LOG(TRACE, "texture " << block << " not found");
		lastBound = {0, 0, SINGLE_TEXTURE, 0, 0, 1, 1};
		return oldTex != 0;
	} else {
		lastBound = iter->second;
	}

	return oldTex != lastBound.tex;
}

GLuint TextureManager::getTexture() {
	return lastBound.tex;
}

void TextureManager::getTextureVertices(vec2f out[4]) const {
	float x = lastBound.x;
	float y = lastBound.y;
	float w = lastBound.w;
	float h = lastBound.h;

	if (lastBound.type == WANG_TILES) {
		w *= 0.25;
		h *= 0.25;
	}

	out[0] = {x    , y    };
	out[1] = {x + w, y    };
	out[2] = {x + w, y + h};
	out[3] = {x    , y + h};
}

void TextureManager::getTextureVertices(vec3i64 bc, uint8 dir, vec2f out[4]) const {
	auto scramble = [](vec3i64 bc) -> uint8 {
		static const uint8 shuffle[256] = {
			0xa3,0xd7,0x09,0x83,0xf8,0x48,0xf6,0xf4,
			0xb3,0x21,0x15,0x78,0x99,0xb1,0xaf,0xf9,
			0xe7,0x2d,0x4d,0x8a,0xce,0x4c,0xca,0x2e,
			0x52,0x95,0xd9,0x1e,0x4e,0x38,0x44,0x28,
			0x0a,0xdf,0x02,0xa0,0x17,0xf1,0x60,0x68,
			0x12,0xb7,0x7a,0xc3,0xe9,0xfa,0x3d,0x53,
			0x96,0x84,0x6b,0xba,0xf2,0x63,0x9a,0x19,
			0x7c,0xae,0xe5,0xf5,0xf7,0x16,0x6a,0xa2,
			0x39,0xb6,0x7b,0x0f,0xc1,0x93,0x81,0x1b,
			0xee,0xb4,0x1a,0xea,0xd0,0x91,0x2f,0xb8,
			0x55,0xb9,0xda,0x85,0x3f,0x41,0xbf,0xe0,
			0x5a,0x58,0x80,0x5f,0x66,0x0b,0xd8,0x90,
			0x35,0xd5,0xc0,0xa7,0x33,0x06,0x65,0x69,
			0x45,0x00,0x94,0x56,0x6d,0x98,0x9b,0x76,
			0x97,0xfc,0xb2,0xc2,0xb0,0xfe,0xdb,0x20,
			0xe1,0xeb,0xd6,0xe4,0xdd,0x47,0x4a,0x1d,
			0x42,0xed,0x9e,0x6e,0x49,0x3c,0xcd,0x43,
			0x27,0xd2,0x07,0xd4,0xde,0xc7,0x67,0x18,
			0x89,0xcb,0x30,0x1f,0x8d,0xc6,0x8f,0xaa,
			0xc8,0x74,0xdc,0xc9,0x5d,0x5c,0x31,0xa4,
			0x70,0x88,0x61,0x2c,0x9f,0x0d,0x2b,0x87,
			0x50,0x82,0x54,0x64,0x26,0x7d,0x03,0x40,
			0x34,0x4b,0x1c,0x73,0xd1,0xc4,0xfd,0x3b,
			0xcc,0xfb,0x7f,0xab,0xe6,0x3e,0x5b,0xa5,
			0xad,0x04,0x23,0x9c,0x14,0x51,0x22,0xf0,
			0x29,0x79,0x71,0x7e,0xff,0x8c,0x0e,0xe2,
			0x0c,0xef,0xbc,0x72,0x75,0x6f,0x37,0xa1,
			0xec,0xd3,0x8e,0x62,0x8b,0x86,0x10,0xe8,
			0x08,0x77,0x11,0xbe,0x92,0x4f,0x24,0xc5,
			0x32,0x36,0x9d,0xcf,0xf3,0xa6,0xbb,0xac,
			0x5e,0x6c,0xa9,0x13,0x57,0x25,0xb5,0xe3,
			0xbd,0xa8,0x3a,0x01,0x05,0x59,0x2a,0x46
		};

		const uint8 *data = reinterpret_cast<uint8 *>(&bc);
		uint32 hash = 1;
		for (uint i = 0; i < sizeof (vec3i64); ++i) {
			hash *= 31;
			hash += data[i];
			hash ^= shuffle[ data[i] & 0xFF ];
		}
		return hash ^ (hash >> 8) ^ (hash >> 16) ^ (hash >> 24);
	};

	float x = lastBound.x;
	float y = lastBound.y;
	float w = lastBound.w;
	float h = lastBound.h;

	if (lastBound.type == WANG_TILES) {
		enum { RIGHT, BACK, TOP, LEFT, FRONT, BOTTOM };
		uint8 left, right,  top, bot;
		switch (dir) {
		case RIGHT:
			left   = scramble(bc + vec3i64(1, 0, 0)) & 0x01;
			right  = scramble(bc + vec3i64(1, 1, 0)) & 0x01;
			bot    = scramble(bc + vec3i64(0, 0, 0)) & 0x02;
			top    = scramble(bc + vec3i64(0, 0, 1)) & 0x02;
			break;
		case LEFT:
			left   = scramble(bc + vec3i64(0, 1, 0)) & 0x01;
			right  = scramble(bc + vec3i64(0, 0, 0)) & 0x01;
			bot    = scramble(bc + vec3i64(0, 0, 0)) & 0x02;
			top    = scramble(bc + vec3i64(0, 0, 1)) & 0x02;
			break;
		case BACK:
			left   = scramble(bc + vec3i64(1, 1, 0)) & 0x01;
			right  = scramble(bc + vec3i64(0, 1, 0)) & 0x01;
			bot    = scramble(bc + vec3i64(0, 0, 0)) & 0x02;
			top    = scramble(bc + vec3i64(0, 0, 1)) & 0x02;
			break;
		case FRONT:
			left   = scramble(bc + vec3i64(0, 0, 0)) & 0x01;
			right  = scramble(bc + vec3i64(1, 0, 0)) & 0x01;
			bot    = scramble(bc + vec3i64(0, 0, 0)) & 0x02;
			top    = scramble(bc + vec3i64(0, 0, 1)) & 0x02;
			break;
		case TOP:
			left   = scramble(bc + vec3i64(0, 0, 0)) & 0x04;
			right  = scramble(bc + vec3i64(1, 0, 0)) & 0x04;
			bot    = scramble(bc + vec3i64(0, 0, 0)) & 0x08;
			top    = scramble(bc + vec3i64(0, 1, 0)) & 0x08;
			break;
		case BOTTOM:
			left   = scramble(bc + vec3i64(1, 0, 0)) & 0x10;
			right  = scramble(bc + vec3i64(0, 0, 0)) & 0x10;
			bot    = scramble(bc + vec3i64(0, 0, 0)) & 0x20;
			top    = scramble(bc + vec3i64(0, 1, 0)) & 0x20;
			break;
		}

		w *= 0.25;
		h *= 0.25;

		if      ( left && !right) x += w * 1;
		else if (!left && !right) x += w * 2;
		else if (!left &&  right) x += w * 3;
		if      ( bot  && !top)   y += h * 1;
		else if (!bot  && !top)   y += h * 2;
		else if (!bot  &&  top)   y += h * 3;
	} // wang tiles

	out[0] = {x    , y    };
	out[1] = {x + w, y    };
	out[2] = {x + w, y + h};
	out[3] = {x    , y + h};
}
